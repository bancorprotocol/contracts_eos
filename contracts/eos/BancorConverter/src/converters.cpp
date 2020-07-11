[[eosio::action]]
void BancorConverter::create(name owner, symbol_code token_code, double initial_supply) {
    require_auth(owner);

    check( token_code.is_valid(), "token_code is invalid");
    symbol token_symbol = symbol(token_code, DEFAULT_TOKEN_PRECISION);
    const double maximum_supply = DEFAULT_MAX_SUPPLY;

    settings settings_table(get_self(), get_self().value);
    const auto& st = settings_table.get();

    converters converters_table(get_self(), get_self().value);
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

    const asset initial_supply_asset = double_to_asset(initial_supply, token_symbol);
    const asset maximum_supply_asset = double_to_asset(maximum_supply, token_symbol);

    // create
    Token::create_action create( st.multi_token, { st.multi_token, "active"_n });
    create.send(get_self(), maximum_supply_asset);

    // issue
    Token::issue_action issue( st.multi_token, { get_self(), "active"_n });
    issue.send(get_self(), initial_supply_asset, "setup");

    // transfer
    Token::transfer_action transfer( st.multi_token, { get_self(), "active"_n });
    transfer.send(get_self(), owner, initial_supply_asset, "setup");

    // MIGRATE DATA to V2
    migrate_converters_v2( token_code );
}

[[eosio::action]]
void BancorConverter::activate( const symbol_code currency, const name protocol_feature, const bool enabled ) {
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
void BancorConverter::delconverter(symbol_code converter_currency_code) {
    converters converters_table(get_self(), get_self().value);
    reserves reserves_table(get_self(), converter_currency_code.raw());
    check(reserves_table.begin() == reserves_table.end(), "delete reserves first");

    const auto& converter = converters_table.get(converter_currency_code.raw(), "converter does not exist");
    converters_table.erase(converter);

    // DELETE MIGRATED DATA from V2
    delete_converters_v2( converter_currency_code );
}
