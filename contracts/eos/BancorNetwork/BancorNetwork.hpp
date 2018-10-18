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

        ACTION init();
        void transfer(name from, name to, asset quantity, string memo);
};
