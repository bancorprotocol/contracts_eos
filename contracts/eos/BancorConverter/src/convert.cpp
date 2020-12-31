void BancorConverter::convert(name from, asset quantity, string memo, name code) {
    BancorConverter::settings _settings(get_self(), get_self().value);
    BancorConverter::converters_v2 _converters( get_self(), get_self().value );

    const auto& settings = _settings.get();
    check(from == settings.network, "converter can only receive from network contract");

    const memo_structure memo_object = parse_memo(memo);
    check(memo_object.path.size() > 1, "invalid memo format");
    check(memo_object.converters[0].account == get_self(), "wrong converter");

    const symbol_code from_path_currency = quantity.symbol.code();
    const symbol_code to_path_currency = symbol_code(memo_object.path[1].c_str());

    const symbol_code converter_currency_code = symbol_code(memo_object.converters[0].sym);
    const auto& converter = _converters.get(converter_currency_code.raw(), "converter does not exist");

    check(from_path_currency != to_path_currency, "cannot convert equivalent currencies");
    check(
        (quantity.symbol == converter.currency && code == settings.multi_token) ||
        code == get_reserve(converter.currency.code(), from_path_currency).contract
        , "unknown 'from' contract");

    const extended_asset from_token = extended_asset(quantity, code);
    extended_symbol to_token;
    if (to_path_currency == converter.currency.code()) {
        check(memo_object.path.size() == 2, "smart token must be final currency");
        to_token = extended_symbol(converter.currency, settings.multi_token);
    }
    else {
        const BancorConverter::reserve r = get_reserve(converter.currency.code(), to_path_currency);
        to_token = extended_symbol(r.balance.symbol, r.contract);
    }

    auto [to_return, fee] = calculate_return(from_token, to_token, memo, converter.currency, converter.fee, settings.multi_token);
    apply_conversion(memo_object, from_token, extended_asset(to_return, to_token.get_contract()), converter.currency);

    emit_conversion_event(
        converter.currency.code(), memo, from_token.contract, from_path_currency, to_token.get_contract(), to_path_currency,
        asset_to_double(quantity),
        asset_to_double(to_return),
        fee
    );
}

std::tuple<asset, double> BancorConverter::calculate_return(const extended_asset from_token, const extended_symbol to_token, const string memo, const symbol currency, const uint64_t fee, const name multi_token) {
    const symbol from_symbol = from_token.quantity.symbol;
    const symbol to_symbol = to_token.get_symbol();

    const bool incoming_smart_token = from_symbol == currency;
    const bool outgoing_smart_token = to_symbol == currency;

    const asset supply = get_supply(multi_token, currency.code());
    double current_smart_supply = supply.amount / pow(10, currency.precision());

    double current_from_balance, current_to_balance;
    BancorConverter::reserve input_reserve, to_reserve;
    if (!incoming_smart_token) {
        input_reserve = get_reserve(currency.code(), from_symbol.code());
        current_from_balance = asset_to_double(input_reserve.balance);
    }
    if (!outgoing_smart_token) {
        to_reserve = get_reserve(currency.code(), to_symbol.code());
        current_to_balance = asset_to_double(to_reserve.balance);
    }
    const bool quick_conversion = !incoming_smart_token && !outgoing_smart_token && input_reserve.weight == to_reserve.weight;

    double from_amount = asset_to_double(from_token.quantity);
    double to_amount;
    if (quick_conversion) { // Reserve --> Reserve
        to_amount = quick_convert(current_from_balance, from_amount, current_to_balance);
    }
    else {
        if (!incoming_smart_token) { // Reserve --> Smart
            to_amount = calculate_purchase_return(current_from_balance, from_amount, current_smart_supply, input_reserve.weight);
            current_smart_supply += to_amount;
            from_amount = to_amount;
        }
        if (!outgoing_smart_token) { // Smart --> Reserve
            to_amount = calculate_sale_return(current_to_balance, from_amount, current_smart_supply, to_reserve.weight);
        }
    }

    const uint8_t magnitude = incoming_smart_token || outgoing_smart_token ? 1 : 2;
    const double calculated_fee = calculate_fee(to_amount, fee, magnitude);
    to_amount -= calculated_fee;

    return std::tuple(
        double_to_asset(to_amount, to_symbol),
        to_fixed(calculated_fee, to_symbol.precision())
    );
}

void BancorConverter::apply_conversion(memo_structure memo_object, extended_asset from_token, extended_asset to_return, symbol converter_currency) {
    path new_path = memo_object.path;
    new_path.erase(new_path.begin(), new_path.begin() + 2);
    memo_object.path = new_path;

    auto new_memo = build_memo(memo_object);

    if (from_token.quantity.symbol == converter_currency) {
        Token::retire_action retire( from_token.contract, { get_self(), "active"_n });
        retire.send(from_token.quantity, "destroy on conversion");
        mod_reserve_balance(converter_currency, -to_return.quantity, -from_token.quantity.amount);
    }
    else if (to_return.quantity.symbol == converter_currency) {
        mod_reserve_balance(converter_currency, from_token.quantity, to_return.quantity.amount);
        Token::issue_action issue( to_return.contract, { get_self(), "active"_n });
        issue.send(get_self(), to_return.quantity, new_memo);
    }
    else {
        mod_reserve_balance(converter_currency, from_token.quantity);
        mod_reserve_balance(converter_currency, -to_return.quantity);
    }

    check(to_return.quantity.amount > 0, "below min return");

    BancorConverter::settings _settings(get_self(), get_self().value);
    const auto settings = _settings.get();
    Token::transfer_action transfer( to_return.contract, { get_self(), "active"_n });
    transfer.send(get_self(), settings.network, to_return.quantity, new_memo);

    // sync (MIGRATION ONLY)
    BancorConverter::synctable_action synctable( get_self(), { get_self(), "active"_n });
    synctable.send( converter_currency.code() );
}