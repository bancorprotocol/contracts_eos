#include "./BancorNetwork.hpp"
#include "../Common/common.hpp"
#include "../BancorConverter/BancorConverter.hpp"

using namespace eosio;

ACTION BancorNetwork::init() {
    require_auth(_self);
}

void BancorNetwork::transfer(name from, name to, asset quantity, string memo) {
    // avoid unstaking and system contract ops mishaps
    if (to != _self || from == "eosio.ram"_n || from == "eosio.stake"_n)
        return;
    
    check(quantity.symbol.is_valid(), "invalid quantity in transfer");
    check(quantity.amount != 0, "zero quantity is disallowed in transfer");

    auto memo_object = parse_memo(memo);
    check(memo_object.path.size() >= 2, "bad path format");

    name next_converter = memo_object.converters[0];
    check(isConverter(next_converter), "converter doesn\'t exist");

    const name destination_account = name(memo_object.dest_account.c_str());
    // the 'from' param must be either the destination account, or a valid, whitelisted converter (in case it's a "2-hop" conversion path)
    if (from != destination_account && destination_account != BANCOR_X) {
        check(isConverter(from), "the destination account must by either the sender, or the BancorX contract account");
    }
    
    action(
        permission_level{ _self, "active"_n },
        _first_receiver, "transfer"_n,
        std::make_tuple(_self, next_converter, quantity, memo)
    ).send();
}

bool BancorNetwork::isConverter(name converter) {
    BancorConverter::settings settings_table(converter, converter.value);
    bool settings_exists = settings_table.exists();
    if (!settings_exists)
        return false;
    
    const auto& st = settings_table.get();

    return st.enabled;
}

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    if (action == "transfer"_n.value && code != receiver)
        eosio::execute_action( eosio::name(receiver), eosio::name(code), &BancorNetwork::transfer );
    if (code == receiver)
        switch( action ) { 
            EOSIO_DISPATCH_HELPER( BancorNetwork, (init) ) 
        }    
}
