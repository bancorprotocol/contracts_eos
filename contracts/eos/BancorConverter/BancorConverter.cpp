
/**
 *  @file
 *  @copyright defined in ../../../LICENSE
 */

#include "../Common/common.hpp"
#include "../Token/Token.hpp"
#include "BancorConverter.hpp"

ACTION BancorConverter::init(name smart_contract, asset smart_currency, bool smart_enabled, bool enabled, name network, uint64_t max_fee, uint64_t fee) {
    require_auth(get_self());
    check(max_fee <= MAX_FEE, 
         ("maximum fee must be lower or equal to " + std::to_string(MAX_FEE)).c_str()
    ); 
    check(fee <= max_fee, "fee must be lower or equal to the maximum fee");

    settings settings_table(get_self(), get_self().value);
    auto st = settings_table.find("settings"_n.value);
    check(st == settings_table.end(), "settings already exist");
    check(is_account(smart_contract), "invalid relay token account");
    
    st = settings_table.emplace(get_self(), [&](auto& s) {		
        s.smart_contract  = smart_contract;		
        s.smart_currency  = smart_currency;		
        s.smart_enabled   = smart_enabled;		
        s.enabled         = enabled;		
        s.network         = network;		
        s.max_fee         = max_fee;		
        s.fee             = fee;		
    });
}

ACTION BancorConverter::update(bool smart_enabled, bool enabled, uint64_t fee) {
    require_auth(get_self());

    settings settings_table(get_self(), get_self().value);
    const auto& st = settings_table.get("settings"_n.value, "settings do not exist");
    
    check(fee <= st.max_fee, "fee must be lower or equal to the maximum fee");
    uint64_t prevFee = st.fee;

    settings_table.modify(st, get_self(), [&](auto& s) {
        s.smart_enabled   = smart_enabled;		
        s.enabled         = enabled;				
        s.fee             = fee;		
    });
    if (prevFee != fee) // trigger the conversion fee update event
        EMIT_CONVERSION_FEE_UPDATE_EVENT(prevFee, fee);
}

ACTION BancorConverter::setreserve(name contract, symbol currency, uint64_t ratio, bool sale_enabled) {
    require_auth(get_self());
    check(currency.is_valid(), "invalid symbol");
    check(is_account(contract), "token's contract is not an account");
    check(ratio > 0 && ratio <= MAX_RATIO,
         ("ratio must be between 1 and " + std::to_string(MAX_RATIO)).c_str());

    reserves reserves_table(get_self(), get_self().value);
    auto existing = reserves_table.find(currency.code().raw());
    if (existing != reserves_table.end()) {
        check(existing->contract == contract, "cannot update the reserve contract name");

        reserves_table.modify(existing, get_self(), [&](auto& s) {
            s.ratio = ratio;
            s.sale_enabled = sale_enabled;
        });
    }
    else reserves_table.emplace(get_self(), [&](auto& s) {
        s.contract  = contract;
        s.currency  = asset(0, currency);
        s.ratio     = ratio;
        s.sale_enabled = sale_enabled;
    });
    uint64_t total_ratio = 0;
    for (auto& reserve : reserves_table)
        total_ratio += reserve.ratio;
    
    check(total_ratio <= MAX_RATIO, 
         ("ratio must be between 1 and " + std::to_string(MAX_RATIO)).c_str());

    settings settings_table(get_self(), get_self().value);
    const auto& converter_settings = settings_table.get("settings"_n.value, "settings do not exist");

    auto current_smart_supply = (get_supply(converter_settings.smart_contract, converter_settings.smart_currency.symbol.code())).amount + converter_settings.smart_currency.amount;
    current_smart_supply /= pow(10, converter_settings.smart_currency.symbol.precision());
    auto reserve_balance = get_balance_amount(contract, get_self(), currency.code()) / pow(10, currency.precision()); 
    EMIT_PRICE_DATA_EVENT(current_smart_supply, contract, currency.code(), reserve_balance, ratio / MAX_RATIO);
}

ACTION BancorConverter::delreserve(symbol_code currency) {
    require_auth(get_self());
    check(currency.is_valid(), "invalid symbol");

    reserves reserves_table(get_self(), get_self().value);
    const auto& rsrv = reserves_table.get(currency.raw(), "reserve not found");
    
    asset balance = get_balance(rsrv.contract, get_self(), currency);
    check(!balance.amount, "may delete only empty reserves");

    reserves_table.erase(rsrv);
}

void BancorConverter::convert(name from, eosio::asset quantity, std::string memo, name code) {
    auto from_amount = quantity.amount / pow(10, quantity.symbol.precision());

    auto memo_object = parse_memo(memo);
    check(memo_object.path.size() > 1, "invalid memo format");
    
    settings settings_table(get_self(), get_self().value);
    const auto& converter_settings = settings_table.get("settings"_n.value, "settings do not exist");
    
    check(converter_settings.enabled, "converter is disabled");
    check(converter_settings.network == from, "converter can only receive from network contract");

    auto contract_name = name(memo_object.path[0].c_str());
    auto from_path_currency = quantity.symbol.code().raw();
    auto to_path_currency = symbol_code(memo_object.path[1].c_str()).raw();

    check(contract_name == get_self(), "wrong converter");    
    check(from_path_currency != to_path_currency, "cannot convert to self");
    
    auto smart_symbol_name = converter_settings.smart_currency.symbol.code().raw();
    auto from_token = get_reserve(from_path_currency, converter_settings);
    auto to_token = get_reserve(to_path_currency, converter_settings);

    auto from_currency = from_token.currency;
    auto to_currency = to_token.currency;

    auto to_currency_precision = to_currency.symbol.precision();
    
    auto from_contract = from_token.contract;
    auto to_contract = to_token.contract;

    bool incoming_smart_token = (from_currency.symbol.code().raw() == smart_symbol_name);
    bool outgoing_smart_token = (to_currency.symbol.code().raw() == smart_symbol_name);
    
    auto from_ratio = from_token.ratio;
    auto to_ratio = to_token.ratio;

    check(to_token.sale_enabled, "'to' token purchases disabled");
    check(code == from_contract, "unknown 'from' contract");
    
    auto current_from_balance = ((get_balance(from_contract, get_self(), from_currency.symbol.code())).amount + from_currency.amount - quantity.amount) / pow(10, from_currency.symbol.precision()); 
    auto current_to_balance = ((get_balance(to_contract, get_self(), to_currency.symbol.code())).amount + to_currency.amount) / pow(10, to_currency_precision);
    
    double current_smart_supply = (get_supply(converter_settings.smart_contract, converter_settings.smart_currency.symbol.code())).amount + converter_settings.smart_currency.amount;
    current_smart_supply /= pow(10, converter_settings.smart_currency.symbol.precision());

    name final_to = name(memo_object.dest_account.c_str());
    
    double smart_tokens = 0;
    double to_tokens = 0;
    bool quick = false;
    
    if (incoming_smart_token) {
        action( // destory received token
            permission_level{ get_self(), "active"_n },
            converter_settings.smart_contract, "retire"_n,
            std::make_tuple(quantity, std::string("destroy on conversion"))
        ).send();

        smart_tokens = from_amount;
    }
    else if (!incoming_smart_token && !outgoing_smart_token && (from_ratio == to_ratio)) {
        to_tokens = quick_convert(current_from_balance, from_amount, current_to_balance);
        quick = true;
    }
    else {
        smart_tokens = calculate_purchase_return(current_from_balance, from_amount, current_smart_supply, from_ratio);
        current_smart_supply += smart_tokens;
    }
    auto issue = false;

    if (outgoing_smart_token) {
        check(memo_object.path.size() == 2, "smart token must be final currency");
        to_tokens = smart_tokens;
        issue = true;
    }
    else if (!quick) {
        to_tokens = calculate_sale_return(current_to_balance, smart_tokens, current_smart_supply, to_ratio);
        current_smart_supply -= smart_tokens;
    }
    uint8_t magnitude = (incoming_smart_token || outgoing_smart_token) ? 1 : 2;
    double fee = calculate_fee(to_tokens, converter_settings.fee, magnitude);

    to_tokens -= fee;

    if (outgoing_smart_token)
        current_smart_supply -= fee;
    
    double formatted_total_fee_amount = to_fixed(fee, to_currency_precision);
    
    to_tokens = to_fixed(to_tokens, to_currency_precision);

    EMIT_CONVERSION_EVENT(memo, from_token.contract, from_currency.symbol.code(), to_token.contract, to_currency.symbol.code(), from_amount, to_tokens, formatted_total_fee_amount);

    if (!incoming_smart_token)
        EMIT_PRICE_DATA_EVENT(current_smart_supply, from_token.contract, from_currency.symbol.code(), current_from_balance + from_amount, from_ratio / MAX_RATIO);
    if (!outgoing_smart_token)
        EMIT_PRICE_DATA_EVENT(current_smart_supply, to_token.contract, to_currency.symbol.code(), current_to_balance - to_tokens, to_ratio / MAX_RATIO);

    path new_path = memo_object.path;
    new_path.erase(new_path.begin(), new_path.begin() + 2);
    memo_object.path = new_path;

    auto new_memo = build_memo(memo_object);

    uint64_t to_amount = to_tokens * pow(10, to_currency_precision);
    asset new_asset = asset(to_amount, to_currency.symbol);
    name inner_to = converter_settings.network;

    if (issue)
        action(
            permission_level{ get_self(), "active"_n },
            to_contract, "issue"_n,
            std::make_tuple(get_self(), new_asset, new_memo) 
        ).send();
    
    check(new_asset.amount > 0, "below min return");
    action(
        permission_level{ get_self(), "active"_n },
        to_contract, "transfer"_n,
        std::make_tuple(get_self(), inner_to, new_asset, new_memo)
    ).send();
}

// returns a reserve object
// can also be called for the smart token itself
const BancorConverter::reserve_t& BancorConverter::get_reserve(uint64_t name, const settings_t& settings) {
    if (settings.smart_currency.symbol.code().raw() == name) {
        static reserve_t temp_reserve;
        temp_reserve.ratio = 0;
        temp_reserve.contract = settings.smart_contract;
        temp_reserve.currency = settings.smart_currency;
        temp_reserve.sale_enabled = settings.smart_enabled;
        return temp_reserve;
    }
    reserves reserves_table(get_self(), get_self().value);
    auto existing = reserves_table.find(name);
    check(existing != reserves_table.end(), "reserve not found");
    return *existing;
}

// returns the balance object for an account
asset BancorConverter::get_balance(name contract, name owner, symbol_code sym) {
    Token::accounts accountstable(contract, owner.value);
    const auto& ac = accountstable.get(sym.raw());
    return ac.balance;
}

// returns the balance amount for an account
uint64_t BancorConverter::get_balance_amount(name contract, name owner, symbol_code sym) {
    Token::accounts accountstable(contract, owner.value);

    auto ac = accountstable.find(sym.raw());
    if (ac != accountstable.end())
        return ac->balance.amount;

    return 0;
}

// returns a token supply
asset BancorConverter::get_supply(name contract, symbol_code sym) {
    Token::stats statstable(contract, sym.raw());
    const auto& st = statstable.get(sym.raw());
    return st.supply;
}

// given a token supply, reserve balance, ratio and a input amount (in the reserve token),
// calculates the return for a given conversion (in the main token)
double BancorConverter::calculate_purchase_return(double balance, double deposit_amount, double supply, int64_t ratio) {
    double R(supply);
    double C(balance);
    double F(ratio / MAX_RATIO);
    double T(deposit_amount);
    double ONE(1.0);

    double E = -R * (ONE - pow(ONE + T / C, F));
    return E;
}

// given a token supply, reserve balance, ratio and a input amount (in the main token),
// calculates the return for a given conversion (in the reserve token)
double BancorConverter::calculate_sale_return(double balance, double sell_amount, double supply, int64_t ratio) {
    double R(supply);
    double C(balance);
    double F(MAX_RATIO / ratio);
    double E(sell_amount);
    double ONE(1.0);

    double T = C * (ONE - pow(ONE - E/R, F));
    return T;
}

double BancorConverter::quick_convert(double balance, double in, double toBalance) {
    return in / (balance + in) * toBalance;
}

void BancorConverter::on_transfer(name from, name to, asset quantity, string memo) {
    check(quantity.is_valid(), "invalid quantity");
    check(quantity.amount > 0, "zero quantity is disallowed");

    // avoid unstaking and system contract ops mishaps
    if (from == get_self() || from == "eosio.ram"_n || from == "eosio.stake"_n || from == "eosio.rex"_n) 
	    return;

    if (memo == "setup") {
        settings settings_table(get_self(), get_self().value);
        const auto& converter_settings = settings_table.get("settings"_n.value, "settings do not exist");
        const auto& reserve = get_reserve(quantity.symbol.code().raw(), converter_settings);

        auto current_smart_supply = (get_supply(converter_settings.smart_contract, converter_settings.smart_currency.symbol.code())).amount + converter_settings.smart_currency.amount;
        current_smart_supply /= pow(10, converter_settings.smart_currency.symbol.precision());
        auto reserve_balance = get_balance_amount(reserve.contract, get_self(), quantity.symbol.code()) / pow(10, quantity.symbol.precision()); 
        
        EMIT_PRICE_DATA_EVENT(current_smart_supply, reserve.contract, quantity.symbol.code(), reserve_balance, reserve.ratio / MAX_RATIO);
    } else 
        convert(from, quantity, memo, get_first_receiver()); 
}
