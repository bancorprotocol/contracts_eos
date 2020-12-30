void BancorConverter::mod_account_balance(name sender, symbol_code converter_currency_code, asset quantity) {
    accounts acnts(get_self(), sender.value);
    auto key = _by_cnvrt(quantity, converter_currency_code);
    auto index = acnts.get_index<"bycnvrt"_n >();
    auto itr = index.find(key);
    bool exists = itr != index.end();
    uint64_t balance = quantity.amount;
    if (quantity.amount < 0) {
        check(exists, "cannot withdraw non-existant deposit");
        check(itr->quantity >= -quantity, "insufficient balance");
    }
    if (exists) {
        index.modify(itr, same_payer, [&](auto& acnt) {
            acnt.quantity += quantity;
        });
        if (itr->is_empty())
            index.erase(itr);
    } else
        acnts.emplace(get_self(), [&](auto& acnt) {
            acnt.symbl = converter_currency_code;
            acnt.quantity = quantity;
            acnt.id = acnts.available_primary_key();
        });
}

void BancorConverter::mod_balances( name sender, asset quantity, symbol_code converter_currency_code, name code ) {
    BancorConverter::converters_v2 _converters( get_self(), get_self().value );
    const auto converter = _converters.get( converter_currency_code.raw(), "converter not found");
    check( converter.reserve_balances.count( quantity.symbol.code() ), "reserve balance not found");

    const BancorConverter::reserve reserve = get_reserve( converter_currency_code, quantity.symbol.code() );

    if (quantity.amount > 0)
        check(code == reserve.contract, "wrong origin contract for quantity");
    else {
        Token::transfer_action transfer( reserve.contract, { get_self(), "active"_n });
        transfer.send(get_self(), sender, -quantity, "withdrawal");
    }

    if (is_converter_active(converter_currency_code))
        mod_account_balance(sender, converter_currency_code, quantity);
    else {
        check(sender == converter.owner, "only converter owner may fund/withdraw prior to activation");
        mod_reserve_balance(converter.currency, quantity);
    }
}

void BancorConverter::mod_reserve_balance(symbol converter_currency, asset value, int64_t pending_supply_change) {
    // settings
    BancorConverter::settings _settings( get_self(), get_self().value );
    BancorConverter::converters_v2 _converters( get_self(), get_self().value );
    const name multi_token = _settings.get().multi_token;
    const symbol_code reserve_symcode = value.symbol.code();

    // supply
    const asset supply = get_supply(multi_token, converter_currency.code());
    auto current_smart_supply = (supply.amount + pending_supply_change) / pow(10, converter_currency.precision());

    // converter
    const auto itr = _converters.find( converter_currency.raw() );
    check( itr != _converters.end(), "converter not found");
    check( itr->reserve_balances.count( reserve_symcode ), "reserve balance not found");
    check( itr->reserve_balances.at( reserve_symcode ).quantity.symbol == value.symbol, "incompatible symbols");

    // modify reserve balance
    _converters.modify(itr, same_payer, [&](auto& row) {
        row.reserve_balances[ reserve_symcode ].quantity += value;
        check( row.reserve_balances[ reserve_symcode ].quantity.amount >= 0, "insufficient amount in reserve");
    });

    // log event
    auto reserve = get_reserve( converter_currency.code(), reserve_symcode );
    double reserve_balance = reserve.balance.amount / pow(10, reserve.balance.symbol.precision());
    emit_price_data_event(converter_currency.code(), current_smart_supply,
                        reserve.contract, reserve.balance.symbol.code(),
                        reserve_balance, reserve.weight);

}
