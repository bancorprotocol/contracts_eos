[[eosio::action]]
void BancorConverter::migrate( const set<symbol_code> converters )
{
    for ( const symbol_code symcode : converters ) {
        const symbol currency = migrate_converter( symcode );
        migrate_reserve( currency );
    }
}

symbol BancorConverter::migrate_converter( const symbol_code symcode )
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
    // currency to be used for reserve migration
    return converter_itr_scope->currency;
}

void BancorConverter::migrate_reserve( const symbol currency )
{
    // tables
    reserves reserves_table_v1( get_self(), currency.code().raw() );
    reserves_v2 reserves_table_v2( get_self(), get_self().value );

    // create empty map objects of ratios & balances
    map<symbol_code, uint64_t> ratios;
    map<symbol_code, extended_asset> balances;

    for ( const auto reserve: reserves_table_v1 ) {
        const symbol_code symcode = reserve.balance.symbol.code();
        ratios[symcode] = reserve.ratio;
        balances[symcode] = extended_asset{reserve.balance, reserve.contract};
    }

    // iterators
    auto reserve_itr_v2 = reserves_table_v2.find( currency.code().raw() );

    // create v2 reserve to main scope
    if ( reserve_itr_v2 == reserves_table_v2.end() ) {
        reserves_table_v2.emplace(get_self(), [&](auto& row) {
            row.currency = currency;
            row.ratios = ratios;
            row.balances = balances;
        });
    // modify
    } else {
        reserves_table_v2.modify(reserve_itr_v2, get_self(), [&](auto& row) {
            row.currency = currency;
            row.ratios = ratios;
            row.balances = balances;
        });
    }
}