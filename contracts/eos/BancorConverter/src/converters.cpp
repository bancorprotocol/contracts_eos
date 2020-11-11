[[eosio::action]]
void BancorConverter::create(const name owner, const symbol_code token_code, const double initial_supply) {
    require_auth(owner);

    BancorConverter::settings settings_table(get_self(), get_self().value);
    BancorConverter::converters_v2 converters_table(get_self(), get_self().value);


    check( token_code.is_valid(), "token_code is invalid");
    symbol token_symbol = symbol(token_code, DEFAULT_TOKEN_PRECISION);
    const double maximum_supply = DEFAULT_MAX_SUPPLY;
    const auto& st = settings_table.get();

    const auto& converter = converters_table.find(token_symbol.code().raw());

    check(converter == converters_table.end(), "converter for the given symbol already exists");
    check(initial_supply > 0, "must have a non-zero initial supply");
    check(initial_supply / maximum_supply <= MAX_INITIAL_MAXIMUM_SUPPLY_RATIO , "the ratio between initial and max supply is too big");

    converters_table.emplace(owner, [&](auto& c) {
        c.currency = token_symbol;
        c.owner = owner;
        c.protocol_features["stake"_n] = false;
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
void BancorConverter::delconverter( const symbol_code converter_currency_code ) {
    BancorConverter::converters_v2 _converters( get_self(), get_self().value );
    const auto itr = _converters.find( converter_currency_code.raw() );

    check( itr != _converters.end(), "converter not found");
    check( itr->reserve_balances.size() == 0, "delete reserves first");
    check( itr->reserve_weights.size() == 0, "delete reserves first");

    _converters.erase( itr );
}
