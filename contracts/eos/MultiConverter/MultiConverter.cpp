
/**
 *  @file
 *  @copyright defined in ../../../LICENSE
 */

#include "../Token/Token.hpp"
#include "MultiConverter.hpp"

ACTION MultiConverter::create(name owner, symbol_code token_code, double initial_supply) {
    require_auth(owner);

    symbol token_symbol = symbol(token_code, DEFAULT_TOKEN_PRECISION);
    double maximum_supply = DEFAULT_MAX_SUPPLY;

    settings settings_table(get_self(), get_self().value);
    const auto& st = settings_table.get("settings"_n.value, "settings do not exist");

    converters converters_table(get_self(), token_symbol.code().raw());
    const auto& converter = converters_table.find(token_symbol.code().raw());
  
    check(converter == converters_table.end(), "converter for the given symbol already exists");
    check(initial_supply > 0, "must have a non-zero initial supply");
    check(initial_supply / maximum_supply <= MAX_INITIAL_MAXIMUM_SUPPLY_RATIO , "the ratio between initial and max supply is too big");
    
    converters_table.emplace(owner, [&](auto& c) {
        c.currency = token_symbol;
        c.owner = owner;
        c.stake_enabled = false;
        c.fee = 0;
    });

    asset initial_supply_asset = asset(initial_supply * pow(10, token_symbol.precision()) , token_symbol);
    asset maximum_supply_asset = asset(maximum_supply * pow(10, token_symbol.precision()) , token_symbol);

    action( 
        permission_level{ st.multi_token, "active"_n },
        st.multi_token, "create"_n, 
        make_tuple(get_self(), maximum_supply_asset) 
    ).send();

    action( 
        permission_level{ get_self(), "active"_n },
        st.multi_token, "issue"_n, 
        make_tuple(get_self(), initial_supply_asset, string("setup"))
    ).send();

    action(
        permission_level{ get_self(), "active"_n },
        st.multi_token, "transfer"_n,
        make_tuple(get_self(), owner, initial_supply_asset, string("setup"))
    ).send();
}

ACTION MultiConverter::setmaxfee(uint64_t maxfee) {
    require_auth(get_self());
   
    settings settings_table(get_self(), get_self().value);
    const auto& st = settings_table.get("settings"_n.value, "set token contract first");

    string error = string("max fee must be lower or equal to ") + std::to_string(MAX_FEE);
    check(maxfee <= MAX_FEE, error.c_str());
    check(maxfee != st.max_fee, "setting same value as before");
    settings_table.modify(st, same_payer, [&](auto& s) {		
        s.max_fee = maxfee;
    });
}

ACTION MultiConverter::setstaking(name staking) {
    check(is_account(staking), "invalid staking account");
    require_auth(get_self());    
    
    settings settings_table(get_self(), get_self().value);
    const auto& st = settings_table.get("settings"_n.value, "set token contract first");    
    
    check(!is_account(st.staking), "staking contract already set");
    
    settings_table.modify(st, same_payer, [&](auto& s) {		
        s.staking = staking;
    });
}

ACTION MultiConverter::setmultitokn(name multi_token) {   
    check(is_account(multi_token), "invalid multi-token account");
    require_auth(get_self());

    settings settings_table(get_self(), get_self().value);
    auto st = settings_table.find("settings"_n.value);
    
    if (st == settings_table.end()) {
        settings_table.emplace(get_self(), [&](auto& s) {
            s.multi_token = multi_token;
        });
    }
    else {
        check(!is_account(st->multi_token), "can only call setmultitokn once");
        settings_table.modify(st, same_payer, [&](auto& s) {
            s.multi_token = multi_token;        
        });
    }
}

ACTION MultiConverter::setnetwork(name network) {   
    require_auth(get_self());
    check(is_account(network), "network account doesn't exist");

    settings settings_table(get_self(), get_self().value);
    auto st = settings_table.find("settings"_n.value);

    if (st == settings_table.end()) {
        settings_table.emplace(get_self(), [&](auto& s) {
            s.network = network;
        });
    }
    else {
        settings_table.modify(st, same_payer, [&](auto& s) {
            s.network = network;        
        });
    }
}

ACTION MultiConverter::enablestake(symbol_code currency, bool enabled) {
    converters converters_table(get_self(), currency.raw());
    settings settings_table(get_self(), get_self().value);
    const auto& st = settings_table.get("settings"_n.value, "settings do not exist");
    const auto& converter = converters_table.get(currency.raw(), "converter does not exist");
    require_auth(converter.owner);

    check(is_account(st.staking), "set staking account before enabling staking");
    check(enabled != converter.stake_enabled, "setting same value as before");
    
    converters_table.modify(converter, same_payer, [&](auto& c) {
        c.stake_enabled = enabled;        
    });
}

ACTION MultiConverter::updateowner(symbol_code currency, name new_owner) {
    converters converters_table(get_self(), currency.raw());
    const auto& converter = converters_table.get(currency.raw(), "converter does not exist");

    require_auth(converter.owner);
    check(is_account(new_owner), "new owner is not an account");
    check(new_owner != converter.owner, "setting same owner as before");
    converters_table.modify(converter, same_payer, [&](auto& c) {
        c.owner = new_owner;
    });   
}

ACTION MultiConverter::updatefee(symbol_code currency, uint64_t fee) {
    settings settings_table(get_self(), get_self().value);
    converters converters_table(get_self(), currency.raw());
    
    const auto& st = settings_table.get("settings"_n.value, "settings do not exist");
    const auto& converter = converters_table.get(currency.raw(), "converter does not exist");

    if (converter.stake_enabled)
        require_auth(st.staking);
    else
        require_auth(converter.owner);
    
    check(fee <= st.max_fee, "fee must be lower or equal to the maximum fee");
    if (converter.fee != fee) { 
        uint64_t prevFee = converter.fee;
        converters_table.modify(converter, same_payer, [&](auto& c) {
            c.fee = fee;
        });
        EMIT_CONVERSION_FEE_UPDATE_EVENT(currency, prevFee, fee);
    }
}

ACTION MultiConverter::setreserve(symbol_code converter_currency_code, symbol currency, name contract, uint64_t ratio) {
    converters converters_table(get_self(), converter_currency_code.raw());
    const auto& converter = converters_table.get(converter_currency_code.raw(), "converter does not exist");
    require_auth(converter.owner);
    
    settings settings_table(get_self(), get_self().value);
    const auto& st = settings_table.get("settings"_n.value, "settings do not exist");

    string error = string("ratio must be between 1 and ") + std::to_string(MAX_RATIO);
    check(ratio > 0 && ratio <= MAX_RATIO, error.c_str());
    
    check(is_account(contract), "token's contract is not an account");
    check(currency.is_valid(), "invalid reserve symbol");
    
    reserves reserves_table(get_self(), converter_currency_code.raw());
    auto reserve = reserves_table.find(currency.code().raw());
    asset smart_supply = get_supply(st.multi_token, converter.currency.code());

    if (reserve != reserves_table.end()) {
        check(reserve->contract == contract, "cannot update the reserve contract name");
        reserves_table.modify(reserve, get_self(), [&](auto& r) {
            r.ratio = ratio;
        });
        auto reserve_balance = reserve->balance.amount / pow(10, currency.precision()); 
        EMIT_PRICE_DATA_EVENT(converter_currency_code, 
                              smart_supply.amount / pow(10, currency.precision()), 
                              reserve->contract, currency.code(), reserve_balance, ratio);
    } else 
        reserves_table.emplace(converter.owner, [&](auto& r) {
            r.contract = contract;
            r.ratio = ratio;
            r.balance = asset(0, currency);
        });
    
    double total_ratio = 0.0;
    for (auto& reserve : reserves_table)
        total_ratio += reserve.ratio;
    
    check(total_ratio <= MAX_RATIO, "total ratio cannot exceed the maximum ratio");
}

ACTION MultiConverter::delreserve(symbol_code converter, symbol_code reserve) {
    check(!is_converter_active(converter),  "a reserve can only be deleted if it's converter is inactive");
    reserves reserves_table(get_self(), converter.raw());
    const auto& rsrv = reserves_table.get(reserve.raw(), "reserve not found");

    reserves_table.erase(rsrv);
}

ACTION MultiConverter::delconverter(symbol_code converter_currency_code) {
    converters converters_table(get_self(), converter_currency_code.raw());
    reserves reserves_table(get_self(), converter_currency_code.raw());
    check(reserves_table.begin() == reserves_table.end(), "delete reserves first");
    
    const auto& converter = converters_table.get(converter_currency_code.raw(), "converter does not exist");
    converters_table.erase(converter);
}

ACTION MultiConverter::fund(name sender, asset quantity) {
    require_auth(sender);
    check(quantity.is_valid() && quantity.amount > 0, "invalid quantity");
    
    settings settings_table(get_self(), get_self().value);
    converters converters_table(get_self(), quantity.symbol.code().raw());
    const auto& st = settings_table.get("settings"_n.value, "settings do not exist");
    const auto& converter = converters_table.get(quantity.symbol.code().raw(), "converter does not exist");
    
    check(converter.currency == quantity.symbol, "symbol mismatch");
    asset supply = get_supply(st.multi_token, quantity.symbol.code());
    reserves reserves_table(get_self(), quantity.symbol.code().raw());
    
    double total_ratio = 0.0;
    for (const auto& reserve : reserves_table)
        total_ratio += reserve.ratio;
        
    for (auto& reserve : reserves_table) {
        double amount = calculate_fund_cost(quantity.amount, supply.amount, reserve.balance.amount, total_ratio);
        asset reserve_amount = asset(ceil(amount), reserve.balance.symbol);
        
        mod_account_balance(sender, quantity.symbol.code(), -reserve_amount);
        mod_reserve_balance(quantity.symbol, reserve_amount);
    }

    action( // issue new smart tokens to the issuer
        permission_level{ get_self(), "active"_n },
        st.multi_token, "issue"_n, 
        make_tuple(get_self(), quantity, string("fund"))
    ).send();
    action(
        permission_level{ get_self(), "active"_n },
        st.multi_token, "transfer"_n,
        make_tuple(get_self(), sender, quantity, string("fund"))
    ).send();
}

void MultiConverter::liquidate(name sender, asset quantity) {
    settings settings_table(get_self(), get_self().value);
    const auto& st = settings_table.get("settings"_n.value, "settings do not exist");
    check(get_first_receiver() == st.multi_token, "bad origin for this transfer");
    
    asset supply = get_supply(st.multi_token, quantity.symbol.code());
    reserves reserves_table(get_self(), quantity.symbol.code().raw());
    
    double total_ratio = 0.0;
    for (const auto& reserve : reserves_table)
        total_ratio += reserve.ratio;

    for (const auto& reserve : reserves_table) {
        double amount = calculate_liquidate_return(quantity.amount, supply.amount, reserve.balance.amount, total_ratio);
        check(amount > 0, "cannot liquidate amounts less than or equal to 0");
        
        asset reserve_amount = asset(amount, reserve.balance.symbol);
        
        mod_reserve_balance(quantity.symbol, -reserve_amount);
        action(
            permission_level{ get_self(), "active"_n },
            reserve.contract, "transfer"_n,
            make_tuple(get_self(), sender, reserve_amount, string("liquidation"))
        ).send();   
    }
    
    action( // remove smart tokens from circulation
        permission_level{ get_self(), "active"_n },
        st.multi_token, "retire"_n,
        make_tuple(quantity, string("liquidation"))
    ).send();
}

ACTION MultiConverter::withdraw(name sender, asset quantity, symbol_code converter_currency_code) {
    require_auth(sender);
    check(quantity.is_valid() && quantity.amount > 0, "invalid quantity");
    mod_balances(sender, -quantity, converter_currency_code, get_self());
}

double MultiConverter::calculate_liquidate_return(double liquidation_amount, double supply, double reserve_balance, double total_ratio) {
    check(supply > 0, "supply must be greater than zero");
    check(reserve_balance > 0, "reserve_balance must be greater than zero");
    check(liquidation_amount <= supply, "liquidation_amount must be less than or equal to the supply");
    check(total_ratio > 1 && total_ratio <= MAX_RATIO * 2, "total_ratio not in range");
    
    if (liquidation_amount == 0)
        return 0;

    if (liquidation_amount == supply)
        return reserve_balance;
    
    if (total_ratio == MAX_RATIO)
        return liquidation_amount * reserve_balance / supply; 

    return reserve_balance * (1.0 - pow(((supply - liquidation_amount) / supply), (MAX_RATIO / total_ratio)));
}

double MultiConverter::calculate_fund_cost(double funding_amount, double supply, double reserve_balance, double total_ratio) {
    check(supply > 0, "supply must be greater than zero");
    check(reserve_balance > 0, "reserve_balance must be greater than zero");
    check(total_ratio > 1 && total_ratio <= MAX_RATIO * 2, "total_ratio not in range");
    
    if (funding_amount == 0)
        return 0;
    
    if (total_ratio == MAX_RATIO)
        return reserve_balance * funding_amount / supply;

    return reserve_balance * (pow(((supply + funding_amount) / supply), (MAX_RATIO / total_ratio)) - 1.0);
}

void MultiConverter::mod_account_balance(name sender, symbol_code converter_currency_code, asset quantity) {
    accounts acnts(get_self(), sender.value);
    auto key = _by_cnvrt(quantity, converter_currency_code);
    auto index = acnts.get_index<"bycnvrt"_n >();
    auto itr = index.find(key);
    bool exists = itr != index.end();
    uint64_t balance = quantity.amount;
    if (quantity.amount < 0) {
        check(exists, "cannot withdraw non-existant deposit");
        check(itr->quantity >= -quantity, "insufficient balance");
    }
    if (exists) {
        index.modify(itr, same_payer, [&](auto& acnt) {
            acnt.quantity += quantity;
        });
        if (itr->is_empty()) 
            index.erase(itr);    
    } else 
        acnts.emplace(get_self(), [&](auto& acnt) {
            acnt.symbl = converter_currency_code;
            acnt.quantity = quantity;
            acnt.id = acnts.available_primary_key();
        });
}

void MultiConverter::mod_balances(name sender, asset quantity, symbol_code converter_currency_code, name code) {
    reserves reserves_table(get_self(), converter_currency_code.raw());
    const auto& reserve = reserves_table.get(quantity.symbol.code().raw(), "reserve doesn't exist");

    converters converters_table(get_self(), converter_currency_code.raw());
    const auto& converter = converters_table.get(converter_currency_code.raw(), "converter does not exist");

    if (quantity.amount > 0)
        check(code == reserve.contract, "wrong origin contract for quantity");
    else
        action(
            permission_level{ get_self(), "active"_n },
            reserve.contract, "transfer"_n,
            make_tuple(get_self(), sender, -quantity, string("withdrawal"))
        ).send();
    
    if (is_converter_active(converter_currency_code))
        mod_account_balance(sender, converter_currency_code, quantity);
    else {
        check(sender == converter.owner, "only converter owner may fund/withdraw prior to activation");
        mod_reserve_balance(converter.currency, quantity);
    }
}

void MultiConverter::mod_reserve_balance(symbol converter_currency, asset value, int64_t pending_supply_change) {
    settings settings_table(get_self(), get_self().value);
    const auto& st = settings_table.get("settings"_n.value, "settings do not exist");
    asset supply = get_supply(st.multi_token, converter_currency.code());
    auto current_smart_supply = (supply.amount + pending_supply_change) / pow(10, converter_currency.precision());
    
    reserves reserves_table(get_self(), converter_currency.code().raw());
    auto& reserve = reserves_table.get(value.symbol.code().raw(), "reserve not found");
    check(reserve.balance.symbol == value.symbol, "incompatible symbols");
    reserves_table.modify(reserve, same_payer, [&](auto& r) {
        r.balance += value;
    });
    double reserve_balance = reserve.balance.amount;
    check(reserve_balance >= 0, "insufficient amount in reserve");
    reserve_balance /= pow(10, reserve.balance.symbol.precision()); 
    EMIT_PRICE_DATA_EVENT(converter_currency.code(), current_smart_supply, 
                          reserve.contract, reserve.balance.symbol.code(), 
                          reserve_balance, reserve.ratio);
}

void MultiConverter::convert(name from, asset quantity, string memo, name code) {
    settings settings_table(get_self(), get_self().value);
    const auto& settings = settings_table.get("settings"_n.value, "settings do not exist");
    check(from == settings.network, "converter can only receive from network contract");

    memo_structure memo_object = parse_memo(memo);
    check(memo_object.path.size() > 1, "invalid memo format");
    check(memo_object.converters[0].account == get_self(), "wrong converter");

    symbol_code from_path_currency = quantity.symbol.code();
    symbol_code to_path_currency = symbol_code(memo_object.path[1].c_str());

    symbol_code converter_currency_code = symbol_code(memo_object.converters[0].sym);
    converters converters_table(get_self(), converter_currency_code.raw());
    const auto& converter = converters_table.get(converter_currency_code.raw(), "converter does not exist");

    check(from_path_currency != to_path_currency, "cannot convert equivalent currencies");
    check(
        (quantity.symbol == converter.currency && code == settings.multi_token) ||
        code == get_reserve(from_path_currency, converter.currency.code()).contract
        , "unknown 'from' contract");
    
    extended_asset from_token = extended_asset(quantity, code);
    extended_symbol to_token;
    if (to_path_currency == converter.currency.code()) {
        check(memo_object.path.size() == 2, "smart token must be final currency");
        to_token = extended_symbol(converter.currency, settings.multi_token);
    }
    else {
        const reserve_t& r = get_reserve(to_path_currency, converter.currency.code());
        to_token = extended_symbol(r.balance.symbol, r.contract);
    }

    auto [to_return, fee] = calculate_return(from_token, to_token, memo, converter, settings.multi_token);
    apply_conversion(memo_object, from_token, extended_asset(to_return, to_token.get_contract()), converter.currency);
    
    EMIT_CONVERSION_EVENT(
        converter.currency.code(), memo, from_token.contract, from_path_currency, to_token.get_contract(), to_path_currency, 
        quantity.amount / pow(10, quantity.symbol.precision()),
        to_return.amount / pow(10, to_return.symbol.precision()),
        fee
    ); 
}

std::tuple<asset, double> MultiConverter::calculate_return(extended_asset from_token, extended_symbol to_token, string memo, const converter_t& converter, name multi_token) {
    symbol from_symbol = from_token.quantity.symbol;
    symbol to_symbol = to_token.get_symbol();

    bool incoming_smart_token = from_symbol == converter.currency;    
    bool outgoing_smart_token = to_symbol == converter.currency;
        
    asset supply = get_supply(multi_token, converter.currency.code());
    double current_smart_supply = supply.amount / pow(10, converter.currency.precision());
    
    double current_from_balance, current_to_balance;
    reserve_t input_reserve, to_reserve;
    if (!incoming_smart_token) {
        input_reserve = get_reserve(from_symbol.code(), converter.currency.code());
        current_from_balance = input_reserve.balance.amount / pow(10, input_reserve.balance.symbol.precision());
    }
    if (!outgoing_smart_token) {
        to_reserve = get_reserve(to_symbol.code(), converter.currency.code());
        current_to_balance = to_reserve.balance.amount / pow(10, to_reserve.balance.symbol.precision());    
    }
    bool quick_conversion = !incoming_smart_token && !outgoing_smart_token && input_reserve.ratio == to_reserve.ratio;

    double from_amount = from_token.quantity.amount / pow(10, from_symbol.precision());
    double to_amount;
    if (quick_conversion) { // Reserve --> Reserve
        to_amount = quick_convert(current_from_balance, from_amount, current_to_balance);   
    }
    else {
        if (!incoming_smart_token) { // Reserve --> Smart
            to_amount = calculate_purchase_return(current_from_balance, from_amount, current_smart_supply, input_reserve.ratio);
            current_smart_supply += to_amount;
            from_amount = to_amount;
        }
        if (!outgoing_smart_token) { // Smart --> Reserve
            to_amount = calculate_sale_return(current_to_balance, from_amount, current_smart_supply, to_reserve.ratio);
        }
    }

    uint8_t magnitude = incoming_smart_token || outgoing_smart_token ? 1 : 2;
    double fee = calculate_fee(to_amount, converter.fee, magnitude);
    to_amount -= fee;
    
    return std::tuple(
        asset(to_amount * pow(10, to_symbol.precision()), to_symbol),
        to_fixed(fee, to_symbol.precision())
    );
}

void MultiConverter::apply_conversion(memo_structure memo_object, extended_asset from_token, extended_asset to_return, symbol converter_currency) {
    path new_path = memo_object.path;
    new_path.erase(new_path.begin(), new_path.begin() + 2);
    memo_object.path = new_path;
    
    auto new_memo = build_memo(memo_object);                         
    
    if (from_token.quantity.symbol == converter_currency) {
        action(
            permission_level{ get_self(), "active"_n },
            from_token.contract, "retire"_n,
            make_tuple(from_token.quantity, string("destroy on conversion"))
        ).send();
        mod_reserve_balance(converter_currency, -to_return.quantity, -from_token.quantity.amount);
    }
    else if (to_return.quantity.symbol == converter_currency) {
        mod_reserve_balance(converter_currency, from_token.quantity, to_return.quantity.amount);
        action(
            permission_level{ get_self(), "active"_n },
            to_return.contract, "issue"_n,
            make_tuple(get_self(), to_return.quantity, new_memo)
        ).send();
    }
    else {
        mod_reserve_balance(converter_currency, from_token.quantity);
        mod_reserve_balance(converter_currency, -to_return.quantity);
    } 
        
    check(to_return.quantity.amount > 0, "below min return");

    settings settings_table(get_self(), get_self().value);
    const auto& st = settings_table.get("settings"_n.value, "settings do not exist");
    action(
        permission_level{ get_self(), "active"_n },
        to_return.contract, "transfer"_n,
        make_tuple(get_self(), st.network, to_return.quantity, new_memo)
    ).send();
}

bool MultiConverter::is_converter_active(symbol_code converter) {
    reserves reserves_table(get_self(), converter.raw());

    for (auto& reserve : reserves_table) {
        if (reserve.balance.amount == 0)
            return false;
    }
    return true;
}

// returns a reserve object, can also be called for the smart token itself
const MultiConverter::reserve_t& MultiConverter::get_reserve(symbol_code symbl, symbol_code converter_currency) {
    reserves reserves_table(get_self(), converter_currency.raw());
    const auto& reserve = reserves_table.get(symbl.raw(), "reserve not found");
    return reserve;
}

// returns a token supply
asset MultiConverter::get_supply(name contract, symbol_code sym) {
    Token::stats statstable(contract, sym.raw());
    const auto& st = statstable.get(sym.raw(), "no stats found");
    return st.supply;
}

// given a token supply, reserve balance, ratio and a input amount (in the reserve token),
// calculates the return for a given conversion (in the smart token)
double MultiConverter::calculate_purchase_return(double balance, double deposit_amount, double supply, int64_t ratio) {
    double R(supply);
    double C(balance);
    double F(ratio / MAX_RATIO);
    double T(deposit_amount);
    double ONE(1.0);

    double E = -R * (ONE - pow(ONE + T / C, F));
    return E;
}

// given a token supply, reserve balance, ratio and a input amount (in the smart token),
// calculates the return for a given conversion (in the reserve token)
double MultiConverter::calculate_sale_return(double balance, double sell_amount, double supply, int64_t ratio) {
    double R(supply);
    double C(balance);
    double F(MAX_RATIO / ratio);
    double E(sell_amount);
    double ONE(1.0);

    double T = C * (ONE - pow(ONE - E/R, F));
    return T;
}

double MultiConverter::quick_convert(double balance, double in, double toBalance) {
    return in / (balance + in) * toBalance;
}

void MultiConverter::on_transfer(name from, name to, asset quantity, string memo) {    
    require_auth(from);
    // avoid unstaking and system contract ops mishaps
    if (to != get_self() || from == get_self() || 
        from == "eosio.ram"_n || from == "eosio.stake"_n || from == "eosio.rex"_n) return;

    check(quantity.is_valid() && quantity.amount > 0, "invalid quantity");
    
    const auto& splitted_memo = split(memo, ";");
    if (splitted_memo[0] == "fund")
        mod_balances(from, quantity, symbol_code(splitted_memo[1]), get_first_receiver());
    else if (splitted_memo[0] == "liquidate")
        liquidate(from, quantity);
    
    else {
        convert(from, quantity, memo, get_first_receiver()); 
    }
}

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    if (action == "transfer"_n.value && code != receiver) 
        eosio::execute_action(eosio::name(receiver), eosio::name(code), &MultiConverter::on_transfer);

    if (code == receiver)
        switch (action) {
            EOSIO_DISPATCH_HELPER(MultiConverter, (create)(delconverter)
            (setmultitokn)(setstaking)(setmaxfee)(setnetwork)
            (updateowner)(updatefee)(enablestake)
            (setreserve)(delreserve)(withdraw)(fund)) 
        }    
}
