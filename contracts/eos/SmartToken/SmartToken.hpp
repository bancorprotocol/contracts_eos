
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <string>

namespace eosiosystem {
    class system_contract;
}

namespace eosio {
using std::string;

CONTRACT SmartToken : public contract {
    using contract::contract;
    public:
        struct transfer_args {
            name  from;
            name  to;
            asset         quantity;
            string        memo;
        };

        ACTION create(name issuer, asset maximum_supply, bool enabled);

        ACTION issue(name to, asset quantity, string memo);
        ACTION retire(asset quantity, string memo);

        ACTION transfer(name from, name to, asset quantity, string memo);

        ACTION close(name owner, symbol_code symbol);

        inline asset get_supply(symbol_code sym) const;
        inline asset get_balance(name owner, symbol_code sym) const;

    private:
        TABLE account {
            asset balance;
            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };

        TABLE currency_stats {
            asset        supply;
            asset        max_supply;
            name         issuer;
            bool         enabled;
            uint64_t primary_key() const { return supply.symbol.code().raw(); }
        };

        typedef eosio::multi_index<"accounts"_n, account> accounts;
        typedef eosio::multi_index<"stat"_n, currency_stats> stats;

        void sub_balance(name owner, asset value);
        void add_balance(name owner, asset value, name ram_payer);
};

asset SmartToken::get_supply(symbol_code sym) const {
    stats statstable(_self, sym.raw());
    const auto& st = statstable.get(sym.raw());
    return st.supply;
}

asset SmartToken::get_balance(name owner, symbol_code sym) const {
    accounts accountstable(_self, owner.value);
    const auto& ac = accountstable.get(sym.raw());
    return ac.balance;
}

} /// namespace eosio
