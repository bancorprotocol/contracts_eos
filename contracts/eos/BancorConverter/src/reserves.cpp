[[eosio::action]]
void BancorConverter::setreserve( const symbol_code converter_currency_code, const symbol currency, const name contract, const uint64_t ratio ) {
    BancorConverter::converters_v2 _converters( get_self(), get_self().value );
    const auto converter = _converters.find( converter_currency_code.raw() );

    // validate input
    require_auth( converter->owner );
    check( converter != _converters.end(), "converter does not exist");
    check( ratio > 0 && ratio <= PPM_RESOLUTION, "weight must be between 1 and " + std::to_string(PPM_RESOLUTION));
    check( is_account(contract), "token contract is not an account");
    check( currency.is_valid(), "invalid reserve symbol");
    check( !converter->reserve_balances.count( currency.code() ), "reserve already exists");

    _converters.modify(converter, same_payer, [&](auto& row) {
        row.reserve_balances[currency.code()] = {{0, currency}, contract};
        row.reserve_weights[currency.code()] = ratio;

        double total_ratio = 0.0;
        for ( const auto item : row.reserve_weights ) {
            total_ratio += item.second;
        }
        check(total_ratio <= PPM_RESOLUTION, "total ratio cannot exceed the maximum ratio");
    });
}

[[eosio::action]]
void BancorConverter::delreserve( const symbol_code converter, const symbol_code reserve ) {
    check(!is_converter_active(converter),  "a reserve can only be deleted if it's converter is inactive");

    BancorConverter::converters_v2 _converters( get_self(), get_self().value );
    const auto itr = _converters.find( converter.raw() );
    check( itr != _converters.end(), "converter not found");
    check( itr->reserve_balances.count( reserve ), "reserve balance not found");

    _converters.modify(itr, same_payer, [&](auto& row) {
        row.reserve_balances.erase( reserve );
        row.reserve_weights.erase( reserve );
    });
}

[[eosio::action]]
void BancorConverter::fund( const name sender, const asset quantity ) {
    require_auth( sender );

    // tables
    BancorConverter::converters_v2 _converters( get_self(), get_self().value );
    BancorConverter::settings _settings( get_self(), get_self().value );

    // settings
    const name multi_token = _settings.get().multi_token;

    // converter
    const symbol_code converter = quantity.symbol.code();
    const auto itr = _converters.find( converter.raw() );

    // validate input
    check( quantity.is_valid() && quantity.amount > 0, "invalid quantity");
    check( itr != _converters.end(), "converter does not exist");
    check( itr->currency == quantity.symbol, "quantity symbol mismatch converter");

    // reserves
    std::vector<BancorConverter::reserve> reserves = BancorConverter::get_reserves( converter );
    const asset supply = get_supply( multi_token, converter );
    double total_weight = 0.0;

    // calculate total weights
    for ( const BancorConverter::reserve reserve : reserves ) {
        total_weight += reserve.weight;
    }

    // modify balance
    for ( const BancorConverter::reserve reserve : reserves ) {
        double amount = calculate_fund_cost( quantity.amount, supply.amount, reserve.balance.amount, total_weight );
        asset reserve_amount = asset( ceil(amount), reserve.balance.symbol );

        mod_account_balance(sender, quantity.symbol.code(), -reserve_amount);
        mod_reserve_balance(quantity.symbol, reserve_amount);
    }

    // issue new smart tokens to the issuer
    Token::issue_action issue( multi_token, { get_self(), "active"_n });
    issue.send(get_self(), quantity, "fund");

    Token::transfer_action transfer( multi_token, { get_self(), "active"_n });
    transfer.send(get_self(), sender, quantity, "fund");
}

[[eosio::action]]
void BancorConverter::withdraw(name sender, asset quantity, symbol_code converter_currency_code) {
    require_auth(sender);
    check(quantity.is_valid() && quantity.amount > 0, "invalid quantity");
    mod_balances(sender, -quantity, converter_currency_code, get_self());
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

void BancorConverter::liquidate( const name sender, const asset quantity) {
    BancorConverter::settings _settings(get_self(), get_self().value);

    const name multi_token = _settings.get().multi_token;
    check( get_first_receiver() == multi_token, "bad origin for this transfer");

    const symbol_code converter = quantity.symbol.code();
    const asset supply = get_supply( multi_token, converter );
    std::vector<BancorConverter::reserve> reserves = BancorConverter::get_reserves( converter );

    double total_weight = 0.0;
    for (const BancorConverter::reserve reserve : reserves ) {
        total_weight += reserve.weight;
    }

    for (const BancorConverter::reserve reserve : reserves ) {
        double amount = calculate_liquidate_return(quantity.amount, supply.amount, reserve.balance.amount, total_weight);
        check(amount > 0, "cannot liquidate amounts less than or equal to 0");

        asset reserve_amount = asset(amount, reserve.balance.symbol);

        mod_reserve_balance(quantity.symbol, -reserve_amount);
        Token::transfer_action transfer( reserve.contract, { get_self(), "active"_n });
        transfer.send(get_self(), sender, reserve_amount, "liquidation");
    }

    // remove smart tokens from circulation
    Token::retire_action retire( multi_token, { get_self(), "active"_n });
    retire.send(quantity, "liquidation");
}

double BancorConverter::calculate_liquidate_return( const double liquidation_amount, const double supply, const double reserve_balance, const double total_weight) {
    check(supply > 0, "supply must be greater than zero");
    check(reserve_balance > 0, "reserve_balance must be greater than zero");
    check(liquidation_amount <= supply, "liquidation_amount must be less than or equal to the supply");
    check(total_weight > 1 && total_weight <= PPM_RESOLUTION * 2, "total_weight not in range");

    if (liquidation_amount == 0)
        return 0;

    if (liquidation_amount == supply)
        return reserve_balance;

    if (total_weight == PPM_RESOLUTION)
        return liquidation_amount * reserve_balance / supply;

    return reserve_balance * (1.0 - pow(((supply - liquidation_amount) / supply), (PPM_RESOLUTION / total_weight)));
}