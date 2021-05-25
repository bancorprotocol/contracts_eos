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
    else if ( params.staking ) check( is_account( params.staking ), "staking account does not exists");

    // multi_token
    if ( settings.multi_token ) check( params.multi_token == settings.multi_token, "multi_token contract already set");
    else check( is_account( params.multi_token ), "multi_token account does not exists");

    // network
    check( is_account( params.network ), "network account does not exists");

    _settings.set( params, get_self() );
}


[[eosio::action]]
void BancorConverter::updateowner(symbol_code currency, name new_owner) {
    BancorConverter::converters _converters(get_self(), get_self().value);
    const auto& converter = _converters.get(currency.raw(), "converter does not exist");

    require_auth(converter.owner);
    check(is_account(new_owner), "new owner is not an account");
    check(new_owner != converter.owner, "setting same owner as before");
    _converters.modify(converter, same_payer, [&](auto& c) {
        c.owner = new_owner;
    });
}

[[eosio::action]]
void BancorConverter::updatefee(symbol_code currency, uint64_t fee) {
    BancorConverter::settings _settings(get_self(), get_self().value);
    BancorConverter::converters _converters(get_self(), get_self().value);

    const auto& st = _settings.get();
    const auto& converter = _converters.get(currency.raw(), "converter does not exist");

    if (converter.protocol_features.at("stake"_n))
        require_auth(st.staking);
    else
        require_auth(converter.owner);

    check(fee <= st.max_fee, "fee must be lower or equal to the maximum fee");
    if (converter.fee != fee) {
        uint64_t prevFee = converter.fee;
        _converters.modify(converter, same_payer, [&](auto& c) {
            c.fee = fee;
        });
        emit_conversion_fee_update_event(currency, prevFee, fee);
    }
}