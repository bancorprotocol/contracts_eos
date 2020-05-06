[[eosio::action]]
void BancorConverter::migrate( const set<symbol_code> converters )
{
    require_auth( get_self() );

    for ( const symbol_code symcode : converters ) {
        migrate_converters_v1_no_scope( symcode );
        migrate_converters_v2( symcode );
    }
}

[[eosio::action]]
void BancorConverter::delmigrate( const set<symbol_code> converters )
{
    require_auth( get_self() );

    for ( const symbol_code symcode : converters ) {
        delete_converters_v2( symcode );
    }
}

void BancorConverter::migrate_converters_v1_no_scope( const symbol_code symcode )
{
    // tables
    converters converters_table_scope( get_self(), symcode.raw() );
    converters converters_table_no_scope( get_self(), get_self().value );

    // iterators
    auto converter_itr_scope = converters_table_scope.find( symcode.raw() );
    auto converter_itr_no_scope = converters_table_no_scope.find( symcode.raw() );

    // scoped symbol code must exists
    check( converter_itr_scope != converters_table_scope.end(), symcode.to_string() + " currency symbol code does not exists" );

    // create
    if ( converter_itr_no_scope == converters_table_no_scope.end() ) {
        // add v1 converters to main scope
        converters_table_no_scope.emplace(get_self(), [&](auto& row) {
            row.currency = converter_itr_scope->currency;
            row.owner = converter_itr_scope->owner;
            row.stake_enabled = converter_itr_scope->stake_enabled;
            row.fee = converter_itr_scope->fee;
        });
    // modify
    } else {
        converters_table_no_scope.modify(converter_itr_no_scope, get_self(), [&](auto& row) {
            row.currency = converter_itr_scope->currency;
            row.owner = converter_itr_scope->owner;
            row.stake_enabled = converter_itr_scope->stake_enabled;
            row.fee = converter_itr_scope->fee;
        });
    }
}

void BancorConverter::migrate_converters_v2( const symbol_code symcode )
{
    // tables
    reserves reserves_table_v1( get_self(), symcode.raw() );
    converters_v2 converters_v2( get_self(), get_self().value );
    converters converters_table_no_scope( get_self(), get_self().value );

    // get no scope v1 converter
    converter_t converter = converters_table_no_scope.get( symcode.raw() );

    // create empty map objects of weights & balances
    map<symbol_code, uint64_t> reserve_weights;
    map<symbol_code, extended_asset> reserve_balances;

    // iterate over v1 reserves
    for ( const auto reserve: reserves_table_v1 ) {
        const symbol_code symcode = reserve.balance.symbol.code();
        reserve_weights[symcode] = reserve.ratio;
        reserve_balances[symcode] = extended_asset{reserve.balance, reserve.contract};
    }

    // iterators
    auto reserve_itr_v2 = converters_v2.find( symcode.raw() );

    // create v2 reserve to main scope
    if ( reserve_itr_v2 == converters_v2.end() ) {
        converters_v2.emplace(get_self(), [&](auto& row) {
            // primary key
            row.currency = converter.currency;

            // converter
            row.owner = converter.owner;
            row.fee = converter.fee;

            // reserve
            row.reserve_weights = reserve_weights;
            row.reserve_balances = reserve_balances;
        });
    // modify
    } else {
        converters_v2.modify(reserve_itr_v2, get_self(), [&](auto& row) {
            // converter
            row.owner = converter.owner;
            row.fee = converter.fee;

            // reserve
            row.reserve_weights = reserve_weights;
            row.reserve_balances = reserve_balances;
        });
    }
}

void BancorConverter::delete_converters_v2( const symbol_code symcode )
{
    // tables
    converters_v2 converters_v2( get_self(), get_self().value );
    auto itr = converters_v2.find( symcode.raw() );

    if ( itr != converters_v2.end() ) {
        converters_v2.erase( itr );
    }
}