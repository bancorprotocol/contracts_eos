bool BancorConverter::is_converter_active(symbol_code converter) {
    reserves reserves_table(get_self(), converter.raw());

    for (auto& reserve : reserves_table) {
        if (reserve.balance.amount == 0)
            return false;
    }
    return true;
}

// returns a reserve object, can also be called for the smart token itself
const BancorConverter::reserve_t& BancorConverter::get_reserve(symbol_code symbl, symbol_code converter_currency) {
    reserves reserves_table(get_self(), converter_currency.raw());
    const auto& reserve = reserves_table.get(symbl.raw(), "reserve not found");
    return reserve;
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
