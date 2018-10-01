#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/currency.hpp>
#include <eosio.system/eosio.system.hpp>

using namespace eosio;

class BancorNetwork : public eosio::contract {
    public:
        BancorNetwork(account_name self)
            :contract(self),
            _this_contract(self),
            tokens_table(self, self),
            converters_table(self, self)
        {}

        void apply(account_name contract, account_name act);

        // @abi table
        struct converter {
            account_name contract;
            bool         enabled;
            uint64_t primary_key() const { return contract; }
            EOSLIB_SERIALIZE(converter, (contract)(enabled))
        };
      
        // @abi table
        struct token {
            account_name contract;
            bool         enabled;
            uint64_t primary_key() const { return contract; }
            EOSLIB_SERIALIZE(token, (contract)(enabled))
        };
      
        // @abi action
        struct clear {
        };
      
        // @abi action
        struct regtoken {
            account_name contract;
            bool         enabled;
            EOSLIB_SERIALIZE(regtoken, (contract)(enabled))
        };

        // @abi action
        struct regconverter {
            account_name contract;
            bool         enabled;
            EOSLIB_SERIALIZE(regconverter, (contract)(enabled))
        };

    private:
        account_name  _this_contract;
        void on(const currency::transfer& t, account_name code);
        void on_clear(const clear& act);
        void on_reg_converter(const regconverter& sc);
        void on_reg_token(const regtoken& sc);

        multi_index<N(token), token> tokens_table;
        multi_index<N(converter), converter> converters_table;  
};
