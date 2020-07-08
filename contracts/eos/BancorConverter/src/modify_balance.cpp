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

void BancorConverter::mod_balances(name sender, asset quantity, symbol_code converter_currency_code, name code) {
    reserves reserves_table(get_self(), converter_currency_code.raw());
    const auto& reserve = reserves_table.get(quantity.symbol.code().raw(), "reserve doesn't exist");

    converters converters_table(get_self(), get_self().value);
    const auto& converter = converters_table.get(converter_currency_code.raw(), "converter does not exist");

    if (quantity.amount > 0)
        check(code == reserve.contract, "wrong origin contract for quantity");
    else
        Token::transfer_action transfer( reserve.contract, { get_self(), "active"_n });
        transfer.send(get_self(), sender, -quantity, "withdrawal");
    if (is_converter_active(converter_currency_code))
        mod_account_balance(sender, converter_currency_code, quantity);
    else {
        check(sender == converter.owner, "only converter owner may fund/withdraw prior to activation");
        mod_reserve_balance(converter.currency, quantity);
    }
}

void BancorConverter::mod_reserve_balance(symbol converter_currency, asset value, int64_t pending_supply_change) {
    settings settings_table(get_self(), get_self().value);
    const auto& st = settings_table.get();
    asset supply = get_supply(st.multi_token, converter_currency.code());
    auto current_smart_supply = (supply.amount + pending_supply_change) / pow(10, converter_currency.precision());

    reserves reserves_table(get_self(), converter_currency.code().raw());
    auto& reserve = reserves_table.get(value.symbol.code().raw(), "reserve not found");
    check(reserve.balance.symbol == value.symbol, "incompatible symbols");
    reserves_table.modify(reserve, same_payer, [&](auto& r) {
        r.balance += value;
    });
    double reserve_balance = reserve.balance.amount;
    check(reserve_balance >= 0, "insufficient amount in reserve");
    reserve_balance /= pow(10, reserve.balance.symbol.precision());
    EMIT_PRICE_DATA_EVENT(converter_currency.code(), current_smart_supply,
                          reserve.contract, reserve.balance.symbol.code(),
                          reserve_balance, reserve.ratio);
}
