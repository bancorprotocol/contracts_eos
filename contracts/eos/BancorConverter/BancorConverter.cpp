
/**
 *  @file
 *  @copyright defined in ../../../LICENSE
 */

#include "../Token/Token.hpp"
#include "BancorConverter.hpp"
// #include "migrate.cpp"

[[eosio::action]]
void BancorConverter::create(name owner, symbol_code token_code, double initial_supply) {
    require_auth(owner);

    check( token_code.is_valid(), "token_code is invalid");
    symbol token_symbol = symbol(token_code, DEFAULT_TOKEN_PRECISION);
    double maximum_supply = DEFAULT_MAX_SUPPLY;

    settings settings_table(get_self(), get_self().value);
    const auto& st = settings_table.get();

    converters_v2 converters_table(get_self(), get_self().value);
    const auto& converter = converters_table.find(token_symbol.code().raw());

    check(converter == converters_table.end(), "converter for the given symbol already exists");
    check(initial_supply > 0, "must have a non-zero initial supply");
    check(initial_supply / maximum_supply <= MAX_INITIAL_MAXIMUM_SUPPLY_RATIO , "the ratio between initial and max supply is too big");

    converters_table.emplace(owner, [&](auto& c) {
        c.currency = token_symbol;
        c.owner = owner;
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

[[eosio::action]]
void BancorConverter::setsettings( const BancorConverter::settings_t params )
{
    require_auth( get_self() );
    BancorConverter::settings _settings( get_self(), get_self().value );
    const auto settings = _settings.get_or_default();

    // validation of params
    // maxfee
    check(params.max_fee <= MAX_FEE, "maxfee must be lower or equal to " + to_string( MAX_FEE ));

    // staking
    if ( settings.staking ) check( params.staking == settings.staking, "staking contract already set");
    else check( is_account( params.staking ), "staking account does not exists");

    // multi_token
    if ( settings.multi_token ) check( params.multi_token == settings.multi_token, "multi_token contract already set");
    else check( is_account( params.multi_token ), "multi_token account does not exists");

    // network
    check( is_account( params.network ), "network account does not exists");

    _settings.set( params, get_self() );
}

[[eosio::action]]
void BancorConverter::activate( const symbol_code currency, const name protocol_feature, const bool enabled ) {
    // disabled (protocol features have not yet been implemented within this contract)
    check( false, "this feature is currently disabled" );

    converters_v2 converters_v2_table(get_self(), get_self().value);
    settings settings_table(get_self(), get_self().value);

    const auto& st = settings_table.get();
    const auto& converter = converters_v2_table.get(currency.raw(), "converter does not exist");

    // only converter owner can activate
    require_auth(converter.owner);

    // available protocol features
    const set<name> protocol_features = set<name>{"stake"_n};
    check( protocol_features.find( protocol_feature ) != protocol_features.end(), "invalid protocol feature");

    // additional check for `stake` protocol feature
    if ( protocol_feature == "stake"_n ) check( is_account(st.staking), "set staking account before enabling staking");

    // update protocol feature
    converters_v2_table.modify(converter, same_payer, [&](auto& row) {
        check(row.protocol_features[protocol_feature] != enabled, "setting same value as before");
        row.protocol_features[protocol_feature] = enabled;
    });
}

[[eosio::action]]
void BancorConverter::updateowner(symbol_code currency, name new_owner) {
    converters_v2 converters_table(get_self(), get_self().value);
    const auto& converter = converters_table.get(currency.raw(), "converter does not exist");

    require_auth(converter.owner);
    check(is_account(new_owner), "new owner is not an account");
    check(new_owner != converter.owner, "setting same owner as before");
    converters_table.modify(converter, same_payer, [&](auto& c) {
        c.owner = new_owner;
    });
}

[[eosio::action]]
void BancorConverter::updatefee(symbol_code currency, uint64_t fee) {
    settings settings_table(get_self(), get_self().value);
    converters_v2 converters_table(get_self(), get_self().value);

    const auto& st = settings_table.get();
    const auto& converter = converters_table.get(currency.raw(), "converter does not exist");

    if (converter.protocol_features.find("stake"_n) != converter.protocol_features.end())
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

[[eosio::action]]
void BancorConverter::setreserve(symbol_code converter_currency_code, symbol currency, name contract, uint64_t ratio) {
    converters_v2 converters_table(get_self(), get_self().value);
    const auto& converter = converters_table.get(converter_currency_code.raw(), "converter does not exist");
    require_auth(converter.owner);

    const string error = string("ratio must be between 1 and ") + std::to_string(MAX_RATIO);
    check(ratio > 0 && ratio <= MAX_RATIO, error.c_str());

    check(is_account(contract), "token's contract is not an account");
    check(currency.is_valid(), "invalid reserve symbol");
    check(converter.reserve_balances.find(currency.code()) == converter.reserve_balances.end(), "reserve already exists");

    converters_table.modify(converter, converter.owner, [&](auto& r) {
        r.reserve_balances[currency.code()] = extended_asset{asset{0, currency}, contract};
        r.reserve_weights[currency.code()] = ratio;

        // validate max ratio
        double total_ratio = 0.0;
        for (auto& reserve_weight : r.reserve_weights)
            total_ratio += reserve_weight.second;

        check(total_ratio <= MAX_RATIO, "total ratio cannot exceed the maximum ratio");
    });

}

[[eosio::action]]
void BancorConverter::delreserve(symbol_code converter, symbol_code reserve) {
    check(!is_converter_active(converter),  "a reserve can only be deleted if it's converter is inactive");
    converters_v2 converters_table(get_self(), get_self().value);
    const auto& itr = converters_table.get(converter.raw(), "converter does not exist");
    check(itr.reserve_balances.size() == 0, "reserves already empty");

    converters_table.modify(itr, get_self(), [&](auto& row) {
        row.reserve_weights.erase(reserve);
        row.reserve_balances.erase(reserve);
    });
}

[[eosio::action]]
void BancorConverter::delconverter(symbol_code converter_currency_code) {
    converters_v2 converters_table(get_self(), get_self().value);
    const auto& converter = converters_table.get(converter_currency_code.raw(), "converter does not exist");
    check(converter.reserve_balances.size() == 0, "delete reserves first");
}

[[eosio::action]]
void BancorConverter::fund(name sender, asset quantity) {
    require_auth(sender);
    check(quantity.is_valid() && quantity.amount > 0, "invalid quantity");

    settings settings_table(get_self(), get_self().value);
    converters_v2 converters_table(get_self(), get_self().value);
    const auto& st = settings_table.get();
    const auto& converter = converters_table.get(quantity.symbol.code().raw(), "converter does not exist");

    check(converter.currency == quantity.symbol, "symbol mismatch");
    asset supply = get_supply(st.multi_token, quantity.symbol.code());

    double total_weights = 0.0;
    for (const auto& reserve_weight : converter.reserve_weights)
        total_weights += reserve_weight.second;

    for (const auto& reserve_balance : converter.reserve_balances) {
        double amount = calculate_fund_cost(quantity.amount, supply.amount, reserve_balance.second.quantity.amount, total_weights);
        asset reserve_amount = asset(ceil(amount), reserve_balance.second.quantity.symbol);

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

void BancorConverter::liquidate(name sender, asset quantity) {
    settings settings_table(get_self(), get_self().value);
    const auto& st = settings_table.get();
    check(get_first_receiver() == st.multi_token, "bad origin for this transfer");

    asset supply = get_supply(st.multi_token, quantity.symbol.code());
    converters_v2 converters_table(get_self(), get_self().value);
    const auto& converter = converters_table.get(quantity.symbol.code().raw(), "converter does not exist");

    double total_weights = 0.0;
    for (const auto& reserve_weight : converter.reserve_weights)
        total_weights += reserve_weight.second;

    for (const auto& reserve_balance : converter.reserve_balances) {
        double amount = calculate_liquidate_return(quantity.amount, supply.amount, reserve_balance.second.quantity.amount, total_weights);
        check(amount > 0, "cannot liquidate amounts less than or equal to 0");

        asset reserve_amount = asset(amount, reserve_balance.second.quantity.symbol);

        mod_reserve_balance(quantity.symbol, -reserve_amount);
        action(
            permission_level{ get_self(), "active"_n },
            reserve_balance.second.contract, "transfer"_n,
            make_tuple(get_self(), sender, reserve_amount, string("liquidation"))
        ).send();
    }

    action( // remove smart tokens from circulation
        permission_level{ get_self(), "active"_n },
        st.multi_token, "retire"_n,
        make_tuple(quantity, string("liquidation"))
    ).send();
}

[[eosio::action]]
void BancorConverter::withdraw(name sender, asset quantity, symbol_code converter_currency_code) {
    require_auth(sender);
    check(quantity.is_valid() && quantity.amount > 0, "invalid quantity");
    mod_balances(sender, -quantity, converter_currency_code, get_self());
}

double BancorConverter::calculate_liquidate_return(double liquidation_amount, double supply, double reserve_balance, double total_ratio) {
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

double BancorConverter::calculate_fund_cost(double funding_amount, double supply, double reserve_balance, double total_ratio) {
    check(supply > 0, "supply must be greater than zero");
    check(reserve_balance > 0, "reserve_balance must be greater than zero");
    check(total_ratio > 1 && total_ratio <= MAX_RATIO * 2, "total_ratio not in range");

    if (funding_amount == 0)
        return 0;

    if (total_ratio == MAX_RATIO)
        return reserve_balance * funding_amount / supply;

    return reserve_balance * (pow(((supply + funding_amount) / supply), (MAX_RATIO / total_ratio)) - 1.0);
}

void BancorConverter::mod_account_balance(name sender, symbol_code converter_currency_code, asset quantity) {
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

void BancorConverter::mod_balances(name sender, asset quantity, symbol_code converter_currency_code, name code) {
    converters_v2 converters_table(get_self(), get_self().value);
    const auto& converter = converters_table.get(converter_currency_code.raw(), "converter does not exist");
    const name reserve_contract = get_reserve_contract( quantity.symbol.code(), converter_currency_code);

    if (quantity.amount > 0)
        check(code == reserve_contract, "wrong origin contract for quantity");
    else
        action(
            permission_level{ get_self(), "active"_n },
            reserve_contract, "transfer"_n,
            make_tuple(get_self(), sender, -quantity, string("withdrawal"))
        ).send();

    if (is_converter_active(converter_currency_code))
        mod_account_balance(sender, converter_currency_code, quantity);
    else {
        check(sender == converter.owner, "only converter owner may fund/withdraw prior to activation");
        mod_reserve_balance(converter.currency, quantity);
    }
}

void BancorConverter::mod_reserve_balance(symbol converter_currency, asset value, int64_t pending_supply_change) {
    settings settings_table(get_self(), get_self().value);
    const auto& st = settings_table.get();
    asset supply = get_supply(st.multi_token, converter_currency.code());
    auto current_smart_supply = (supply.amount + pending_supply_change) / pow(10, converter_currency.precision());

    converters_v2 converters_table(get_self(), get_self().value);
    auto& converter = converters_table.get(value.symbol.code().raw(), "reserve not found");

    // reserves
    extended_asset reserve_balance = get_reserve_balance(value.symbol.code(), converter_currency.code());
    uint64_t reserve_weight = get_reserve_weight(value.symbol.code(), converter_currency.code());
    check(reserve_balance.quantity.symbol == value.symbol, "incompatible symbols");

    reserve_balance.quantity += value;
    converters_table.modify(converter, same_payer, [&](auto& r) {
        r.reserve_balances[value.symbol.code()] = reserve_balance;
    });
    check(reserve_balance.quantity.amount >= 0, "insufficient amount in reserve");

    EMIT_PRICE_DATA_EVENT(converter_currency.code(), current_smart_supply,
                          reserve_balance.contract, reserve_balance.quantity.symbol.code(),
                          asset_to_double(reserve_balance.quantity), reserve_weight);
}

void BancorConverter::convert(name from, asset quantity, string memo, name code) {
    settings settings_table(get_self(), get_self().value);
    const auto& settings = settings_table.get();
    check(from == settings.network, "converter can only receive from network contract");

    memo_structure memo_object = parse_memo(memo);
    check(memo_object.path.size() > 1, "invalid memo format");
    check(memo_object.converters[0].account == get_self(), "wrong converter");

    symbol_code from_path_currency = quantity.symbol.code();
    symbol_code to_path_currency = symbol_code(memo_object.path[1].c_str());

    symbol_code converter_currency_code = symbol_code(memo_object.converters[0].sym);
    converters_v2 converters_table(get_self(), get_self().value);
    const auto& converter = converters_table.get(converter_currency_code.raw(), "converter does not exist");

    const extended_asset from_reserve_balance = get_reserve_balance(from_path_currency, converter.currency.code());

    check(from_path_currency != to_path_currency, "cannot convert equivalent currencies");
    check(
        (quantity.symbol == converter.currency && code == settings.multi_token) ||
        code == from_reserve_balance.contract
        , "unknown 'from' contract");

    extended_asset from_token = extended_asset(quantity, code);
    extended_symbol to_token;
    if (to_path_currency == converter.currency.code()) {
        check(memo_object.path.size() == 2, "smart token must be final currency");
        to_token = extended_symbol(converter.currency, settings.multi_token);
    }
    else {
        const extended_asset to_reserve_balance = get_reserve_balance(to_path_currency, converter.currency.code());
        to_token = extended_symbol(to_reserve_balance.quantity.symbol, to_reserve_balance.contract);
    }

    auto [to_return, fee] = calculate_return(from_token, to_token, memo, converter, settings.multi_token);
    apply_conversion(memo_object, from_token, extended_asset(to_return, to_token.get_contract()), converter.currency);

    EMIT_CONVERSION_EVENT(
        converter.currency.code(), memo, from_token.contract, from_path_currency, to_token.get_contract(), to_path_currency,
        asset_to_double(quantity),
        asset_to_double(to_return),
        fee
    );
}

std::tuple<asset, double> BancorConverter::calculate_return(extended_asset from_token, extended_symbol to_token, string memo, const converter_v2_t& converter, name multi_token) {
    symbol from_symbol = from_token.quantity.symbol;
    symbol to_symbol = to_token.get_symbol();

    bool incoming_smart_token = from_symbol == converter.currency;
    bool outgoing_smart_token = to_symbol == converter.currency;

    asset supply = get_supply(multi_token, converter.currency.code());
    double current_smart_supply = asset_to_double(supply);

    double current_from_balance, current_to_balance;
    extended_asset input_reserve_balance, to_reserve_balance;
    uint64_t input_reserve_weight, to_reserve_weight;

    if (!incoming_smart_token) {
        input_reserve_balance = get_reserve_balance(from_symbol.code(), converter.currency.code());
        input_reserve_weight = get_reserve_weight(from_symbol.code(), converter.currency.code());
        current_from_balance = asset_to_double(input_reserve_balance.quantity);
    }
    if (!outgoing_smart_token) {
        to_reserve_balance = get_reserve_balance(to_symbol.code(), converter.currency.code());
        to_reserve_weight = get_reserve_weight(to_symbol.code(), converter.currency.code());
        current_to_balance = asset_to_double(input_reserve_balance.quantity);
    }
    bool quick_conversion = !incoming_smart_token && !outgoing_smart_token && input_reserve_weight == to_reserve_weight;

    double from_amount = asset_to_double(from_token.quantity);
    double to_amount;
    if (quick_conversion) { // Reserve --> Reserve
        to_amount = quick_convert(current_from_balance, from_amount, current_to_balance);
    }
    else {
        if (!incoming_smart_token) { // Reserve --> Smart
            to_amount = calculate_purchase_return(current_from_balance, from_amount, current_smart_supply, input_reserve_weight);
            current_smart_supply += to_amount;
            from_amount = to_amount;
        }
        if (!outgoing_smart_token) { // Smart --> Reserve
            to_amount = calculate_sale_return(current_to_balance, from_amount, current_smart_supply, to_reserve_weight);
        }
    }

    uint8_t magnitude = incoming_smart_token || outgoing_smart_token ? 1 : 2;
    double fee = calculate_fee(to_amount, converter.fee, magnitude);
    to_amount -= fee;

    return std::tuple(
        double_to_asset( to_amount, to_symbol ),
        to_fixed(fee, to_symbol.precision())
    );
}

void BancorConverter::apply_conversion(memo_structure memo_object, extended_asset from_token, extended_asset to_return, symbol converter_currency) {
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
    const auto& st = settings_table.get();
    action(
        permission_level{ get_self(), "active"_n },
        to_return.contract, "transfer"_n,
        make_tuple(get_self(), st.network, to_return.quantity, new_memo)
    ).send();
}

bool BancorConverter::is_converter_active(symbol_code converter_currency) {
    converters_v2 converters_table(get_self(), get_self().value);
    const auto converter = converters_table.get(converter_currency.raw(), "converter not found");

    for (auto& reserve_balance : converter.reserve_balances) {
        if (reserve_balance.second.quantity.amount == 0)
            return false;
    }
    return true;
}

// returns a reserve object, can also be called for the smart token itself
extended_asset BancorConverter::get_reserve_balance(symbol_code symbl, symbol_code converter_currency) {
    converters_v2 converters_table(get_self(), get_self().value);
    const auto converter = converters_table.get(converter_currency.raw(), "converter not found");
    const auto reserve_balance = converter.reserve_balances.find(symbl);
    check( reserve_balance != converter.reserve_balances.end(), "reserve not found" );
    return reserve_balance->second;
}

name BancorConverter::get_reserve_contract(symbol_code symbl, symbol_code converter_currency) {
    return get_reserve_balance(symbl, converter_currency).contract;
}

uint64_t BancorConverter::get_reserve_weight(symbol_code symbl, symbol_code converter_currency) {
    converters_v2 converters_table(get_self(), get_self().value);
    const auto converter = converters_table.get(converter_currency.raw(), "converter not found");
    const auto reserve_weight = converter.reserve_weights.find(symbl);
    check( reserve_weight != converter.reserve_weights.end(), "reserve not found" );
    return reserve_weight->second;
}

// returns a token supply
asset BancorConverter::get_supply(name contract, symbol_code sym) {
    Token::stats statstable(contract, sym.raw());
    const auto& st = statstable.get(sym.raw(), "no stats found");
    return st.supply;
}

// given a token supply, reserve balance, ratio and a input amount (in the reserve token),
// calculates the return for a given conversion (in the smart token)
double BancorConverter::calculate_purchase_return(double balance, double deposit_amount, double supply, int64_t ratio) {
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

double BancorConverter::asset_to_double( const asset quantity ) {
    if ( quantity.amount == 0 ) return 0.0;
    return quantity.amount / pow(10, quantity.symbol.precision());
}

asset BancorConverter::double_to_asset( const double amount, const symbol sym ) {
    return asset{ static_cast<int64_t>(amount * pow(10, sym.precision())), sym };
}

[[eosio::on_notify("*::transfer")]]
void BancorConverter::on_transfer(name from, name to, asset quantity, string memo) {
    require_auth(from);

    // avoid unstaking and system contract ops mishaps
    if (to != get_self() || from == get_self() ||
        from == "eosio.ram"_n || from == "eosio.stake"_n || from == "eosio.rex"_n) return;

    check(quantity.is_valid() && quantity.amount > 0, "invalid quantity");

    const auto& splitted_memo = split(memo, ";");

    if (splitted_memo[0] == "fund") {
        mod_balances(from, quantity, symbol_code(splitted_memo[1]), get_first_receiver());
    } else if (splitted_memo[0] == "liquidate") {
        liquidate(from, quantity);
    } else {
        convert(from, quantity, memo, get_first_receiver());
    }
}
