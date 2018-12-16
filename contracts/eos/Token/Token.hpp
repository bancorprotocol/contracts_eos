#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <string>

namespace eosiosystem {
    class system_contract;
}

namespace eosio {
using std::string;

CONTRACT Token : public contract {
    using contract::contract;
    public:
        struct transfer_args {
            name    from;
            name    to;
            asset   quantity;
            string  memo;
        };

        ACTION create(name issuer, asset maximum_supply);

        ACTION issue(name to, asset quantity, string memo);
        ACTION retire(asset quantity, string memo);

        ACTION transfer(name from, name to, asset quantity, string memo);
        ACTION transferbyid(name from, name to, uint64_t amount_id, name contract, string memo);

        ACTION open(name owner, symbol_code symbol, name ram_payer);
        ACTION close(name owner, symbol_code symbol);

        static asset get_supply(name token_contract_account, symbol_code sym) {
            stats statstable(token_contract_account, sym.raw());
            const auto& st = statstable.get(sym.raw());
            return st.supply;
        }

        static asset get_balance(name token_contract_account, name owner, symbol_code sym) {
            accounts accountstable(token_contract_account, owner.value);
            const auto& ac = accountstable.get(sym.raw());
            return ac.balance;
        }

    private:
        TABLE account {
            asset balance;
            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };

        TABLE currency_stats {
            asset   supply;
            asset   max_supply;
            name    issuer;
            uint64_t primary_key() const { return supply.symbol.code().raw(); }
        };

        typedef eosio::multi_index<"accounts"_n, account> accounts;
        typedef eosio::multi_index<"stat"_n, currency_stats> stats;

        asset get_quantity_by_id(name contract, uint64_t id);

        void sub_balance(name owner, asset value);
        void add_balance(name owner, asset value, name ram_payer);
};

} /// namespace eosio
