#include "./BancorNetwork.hpp"
#include "../Common/common.hpp"

using namespace eosio;

void BancorNetwork::transfer(name from, name to, asset quantity, string memo) {
    if (to != _self)
        return;
 
    // auto a = extended_asset(, code);
    eosio_assert(quantity.symbol.is_valid(), "invalid quantity in transfer");
    eosio_assert(quantity.amount != 0, "zero quantity is disallowed in transfer");
    
    auto memoObject = parseMemo(memo);
    eosio_assert(memoObject.path.size() >= 3, "bad path format");
    name convertContract = eosio::name(memoObject.path[0].c_str());
    
    
    
    tokens tokens_table(_self, _self.value);
    converters converters_table(_self, _self.value);
    auto converter_config = converters_table.find(convertContract.value);
    auto token_config = tokens_table.find(_code.value);
    
    eosio_assert(token_config != tokens_table.end(), "unknown token");
    eosio_assert(converter_config != converters_table.end(), "unknown converter");
    
    eosio_assert(token_config->enabled, "token disabled");
    eosio_assert(converter_config->enabled, "convertor disabled");
    
    action(
        permission_level{ _self, "active"_n },
        _code, "transfer"_n,
        std::make_tuple(_self, convertContract, quantity, memo)
    ).send();
}

ACTION BancorNetwork::clear(){
    require_auth(_self);

    tokens tokens_table(_self, _self.value);
    converters converters_table(_self, _self.value);
    
    auto it_converter = converters_table.begin();
    while (it_converter != converters_table.end()) {
        it_converter = converters_table.erase(it_converter);
    }
    auto it_token = tokens_table.begin();
    while (it_token != tokens_table.end()) {
        it_token = tokens_table.erase(it_token);
    }
}

ACTION BancorNetwork::regconverter(name contract,bool enabled){
    require_auth(_self);
    converters converters_table(_self, _self.value);

    auto existing = converters_table.find(contract.value);
    if (existing != converters_table.end()) {
        converters_table.modify(existing,_self, [&](auto& s) {
            s.contract = contract;
            s.enabled = enabled;
        });
    }
    else converters_table.emplace(_self, [&](auto& s) {
        s.contract = contract;
        s.enabled = enabled;
    });
}

        
        
ACTION BancorNetwork::regtoken(name contract, bool enabled) {
    require_auth(_self);
    tokens tokens_table(_self, _self.value);
    auto existing = tokens_table.find(contract.value);
    if (existing != tokens_table.end()) {
        tokens_table.modify(existing, _self, [&](auto& s) {
            s.contract = contract;
            s.enabled = enabled;
        });
    }
    else tokens_table.emplace(_self, [&](auto& s) {
        s.contract = contract;
        s.enabled = enabled;
    });
}

extern "C" {
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        
        
        if (action == "transfer"_n.value && code != receiver) {
            eosio::execute_action( eosio::name(receiver), eosio::name(code), &BancorNetwork::transfer );
        }
        if (code == receiver){
            switch( action ) { 
                EOSIO_DISPATCH_HELPER( BancorNetwork, (regtoken)(regconverter)(clear) ) 
            }    
        }
        eosio_exit(0);
    }
}
