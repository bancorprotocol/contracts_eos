bool BancorConverter::is_converter_active( const symbol_code converter) {
    std::vector<BancorConverter::reserve> reserves = BancorConverter::get_reserves( converter );

    for (const BancorConverter::reserve reserve : reserves) {
        if (reserve.balance.amount == 0)
            return false;
    }
    return true;
}

BancorConverter::reserve BancorConverter::get_reserve( const symbol_code currency, const symbol_code reserve )
{
    BancorConverter::converters_v2 _converter( get_self(), get_self().value );
    auto row = _converter.get( currency.raw(), "BancorConverter: currency symbol does not exist");
    check(row.reserve_balances.count(reserve), "BancorConverter: reserve balance symbol does not exist");
    check(row.reserve_weights.count(reserve), "BancorConverter: reserve weights symbol does not exist");

    const extended_asset balance = row.reserve_balances.at(reserve);
    return BancorConverter::reserve{ balance.contract, row.reserve_weights.at(reserve), balance.quantity };
}

std::vector<BancorConverter::reserve> BancorConverter::get_reserves( const symbol_code currency )
{
    BancorConverter::converters_v2 _converter( get_self(), get_self().value );
    std::vector<BancorConverter::reserve> reserves;

    auto row = _converter.get( currency.raw(), "BancorConverter: currency symbol does not exist");
    for ( const auto itr : row.reserve_balances ) {
        reserves.push_back( BancorConverter::get_reserve( currency, itr.first ) );
    }
    return reserves;
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