#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;

using std::string;
using std::vector;

CONTRACT BancorNetwork : public eosio::contract {
    using contract::contract;
    public:

        TABLE converter_t {
            name contract;
            bool         enabled;
            uint64_t primary_key() const { return contract.value; }
        };
      
        TABLE token_t {
            name contract;
            bool         enabled;
            uint64_t primary_key() const { return contract.value; }
        };

        typedef eosio::multi_index<"token"_n, token_t> tokens;
        typedef eosio::multi_index<"converter"_n, converter_t> converters;
      
        
        ACTION clear();
        ACTION regconverter(name contract,bool enabled);
        ACTION regtoken(name contract, bool enabled);
        void transfer(name from, name to, asset quantity, string memo);

   
  
};
