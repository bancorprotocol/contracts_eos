[[eosio::action]]
void BancorConverter::log( const string event, const string version, const map<string, string> payload ) {
    require_auth( get_self() );
}

void BancorConverter::EMIT_CONVERSION_EVENT(
    const symbol_code converter_currency_symbol,
    const string memo,
    const name from_contract,
    const symbol_code from_symbol,
    const name to_contract,
    const symbol_code to_symbol,
    const double amount,
    const double to_return,
    const double conversion_fee
) {
    // payload
    map<string, string> payload;
    payload["converter_currency_symbol"] = converter_currency_symbol.to_string();
    payload["memo"] = memo;
    payload["from_contract"] = from_contract.to_string();
    payload["from_symbol"] = from_symbol.to_string();
    payload["to_contract"] = to_contract.to_string();
    payload["to_symbol"] = to_symbol.to_string();
    payload["amount"] = to_string(amount);
    payload["return"] = to_string(to_return);
    payload["conversion_fee"] = to_string(conversion_fee);

    // send action
    BancorConverter::log_action log( get_self(), { get_self(), "active"_n });
    log.send("conversion", "1.4", payload);
}

void BancorConverter::EMIT_PRICE_DATA_EVENT(
    const symbol_code converter_currency_symbol,
    const double smart_supply,
    const name reserve_contract,
    const symbol_code reserve_symbol,
    const double reserve_balance,
    const double reserve_ratio
) {
    // payload
    map<string, string> payload;
    payload["converter_currency_symbol"] = converter_currency_symbol.to_string();
    payload["smart_supply"] = to_string(smart_supply);
    payload["reserve_contract"] = reserve_contract.to_string();
    payload["reserve_symbol"] = reserve_symbol.to_string();
    payload["reserve_balance"] = to_string(reserve_balance);
    payload["reserve_ratio"] = to_string(reserve_ratio);

    // send action
    BancorConverter::log_action log( get_self(), { get_self(), "active"_n });
    log.send("price_data", "1.5", payload);
}

void BancorConverter::EMIT_CONVERSION_FEE_UPDATE_EVENT(
    const symbol_code converter_currency_symbol,
    const uint64_t prev_fee,
    const uint64_t new_fee
) {
    // payload
    map<string, string> payload;
    payload["converter_currency_symbol"] = converter_currency_symbol.to_string();
    payload["prev_fee"] = to_string(prev_fee);
    payload["new_fee"] = to_string(new_fee);

    // send action
    BancorConverter::log_action log( get_self(), { get_self(), "active"_n });
    log.send("conversion_fee_update", "1.2", payload);
}
