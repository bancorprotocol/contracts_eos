[[eosio::action]]
void BancorConverter::migrate( const set<symbol_code> currencies )
{
    require_auth( get_self() );

    for ( const symbol_code symcode : currencies ) {
        // TO-DO
        // migrate TABLE `converters.v2` => `converters`
    }
}

[[eosio::action]]
void BancorConverter::cleartables( const set<symbol_code> currencies )
{
    require_auth( get_self() );

    for ( const symbol_code symcode : currencies ) {
        // erase_converters_v1( symcode );
        // erase_converters_v1_scoped( symcode );
        // erase_reserves_v1( symcode );
    }
}

[[eosio::action]]
void BancorConverter::synctables( const set<symbol_code> currencies )
{
    require_auth( get_self() );

    for ( const symbol_code symcode : currencies ) {
        synctable( symcode );
    }
}

[[eosio::action]]
void BancorConverter::synctable( const symbol_code currency )
{
    require_auth( get_self() );

    BancorConverter::converters_v2 _converters_v2(get_self(), get_self().value);
    BancorConverter::converters _converters(get_self(), get_self().value);

    auto itr_v2 = _converters_v2.find( currency.raw() );
    auto itr = _converters.find( currency.raw() );

    // clone previous table data
    auto insert = [&]( auto & row ) {
        row.currency = itr_v2->currency;
        row.owner = itr_v2->owner;
        row.fee = itr_v2->fee;
        row.reserve_weights = itr_v2->reserve_weights;
        row.reserve_balances = itr_v2->reserve_balances;
        row.protocol_features = itr_v2->protocol_features;
        row.metadata_json = itr_v2->metadata_json;
    };

    // create or modify
    if ( itr == _converters.end() ) _converters.emplace( get_self(), insert );
    else _converters.modify( itr, get_self(), insert );
}

// void BancorConverter::erase_converters_v1( const symbol_code symcode )
// {
//     BancorConverter::converters _converters( get_self(), get_self().value );
//     auto itr = _converters.find( symcode.raw() );
//     if ( itr != _converters.end() ) _converters.erase( itr );
// }

// void BancorConverter::erase_converters_v1_scoped( const symbol_code symcode )
// {
//     BancorConverter::converters _converters( get_self(), symcode.raw() );
//     clear_table( _converters );
// }

// void BancorConverter::erase_reserves_v1( const symbol_code symcode )
// {
//     BancorConverter::reserves _reserves( get_self(), symcode.raw() );
//     clear_table( _reserves );
// }

template <typename T>
void BancorConverter::clear_table( T& table )
{
    auto itr = table.begin();
    while ( itr != table.end() ) {
        itr = table.erase( itr );
    }
}