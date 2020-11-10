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
        erase_converters_v1( symcode );
        erase_converters_v1_scoped( symcode );
        erase_reserves_v1( symcode );
    }
}

void BancorConverter::erase_converters_v1( const symbol_code symcode )
{
    BancorConverter::converters _converters( get_self(), get_self().value );
    auto itr = _converters.find( symcode.raw() );
    if ( itr != _converters.end() ) _converters.erase( itr );
}

void BancorConverter::erase_converters_v1_scoped( const symbol_code symcode )
{
    BancorConverter::converters _converters( get_self(), symcode.raw() );
    clear_table( _converters );
}

void BancorConverter::erase_reserves_v1( const symbol_code symcode )
{
    BancorConverter::reserves _reserves( get_self(), symcode.raw() );
    clear_table( _reserves );
}

template <typename T>
void BancorConverter::clear_table( T& table )
{
    auto itr = table.begin();
    while ( itr != table.end() ) {
        itr = table.erase( itr );
    }
}