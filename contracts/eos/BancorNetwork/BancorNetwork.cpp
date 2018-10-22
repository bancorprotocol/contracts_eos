#include "./BancorNetwork.hpp"
#include "../Common/common.hpp"

using namespace eosio;

void BancorNetwork::transfer(name from, name to, asset quantity, string memo) {
    if (to != _self)
        return;
 
    // auto a = extended_asset(, code);
    eosio_assert(quantity.symbol.is_valid(), "invalid quantity in transfer");
    eosio_assert(quantity.amount != 0, "zero quantity is disallowed in transfer");
    
    auto memo_object = parse_memo(memo);
    eosio_assert(memo_object.path.size() >= 2, "bad path format");
    name convert_contract = eosio::name(memo_object.path[0].c_str());

    action(
        permission_level{ _self, "active"_n },
        _code, "transfer"_n,
        std::make_tuple(_self, convert_contract, quantity, memo)
    ).send();
}

ACTION BancorNetwork::init() {
    require_auth(_self);
}

extern "C" {
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {

        if (action == "transfer"_n.value && code != receiver) {
            eosio::execute_action( eosio::name(receiver), eosio::name(code), &BancorNetwork::transfer );
        }
        if (code == receiver){
            switch( action ) { 
                EOSIO_DISPATCH_HELPER( BancorNetwork, (init) ) 
            }    
        }
        eosio_exit(0);
    }
}
