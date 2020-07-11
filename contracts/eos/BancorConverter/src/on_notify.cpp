[[eosio::on_notify("*::transfer")]]
void BancorConverter::on_transfer(name from, name to, asset quantity, string memo) {
    require_auth(from);

    // avoid unstaking and system contract ops mishaps
    if (to != get_self() || from == get_self() ||
        from == "eosio.ram"_n || from == "eosio.stake"_n || from == "eosio.rex"_n) return;

    check(quantity.is_valid() && quantity.amount > 0, "invalid quantity");

    const auto& splitted_memo = split(memo, ";");

    if (splitted_memo[0] == "fund") {
        mod_balances(from, quantity, symbol_code(splitted_memo[1]), get_first_receiver());
    } else if (splitted_memo[0] == "liquidate") {
        liquidate(from, quantity);
    } else {
        convert(from, quantity, memo, get_first_receiver());
    }
}
