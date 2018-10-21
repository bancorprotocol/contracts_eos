#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/symbol.hpp>
#include "../Common/common.hpp"

using namespace eosio;
using std::string;
using std::vector;

#define EMIT_CONVERT_EVENT(memo, current_base_balance, current_target_balance, current_smart_supply, base_amount, smart_tokens, target_amount, base_weight, target_weight, to_connector_max_fee, from_connector_max_fee, to_connector_fee, from_connector_fee) \
    START_EVENT("convert", "1.1") \
    EVENTKV("memo", memo) \
    EVENTKV("current_base_balance", current_base_balance) \
    EVENTKV("current_target_balance", current_target_balance) \
    EVENTKV("current_smart_supply", current_smart_supply) \
    EVENTKV("base_amount", base_amount) \
    EVENTKV("smart_tokens", smart_tokens) \
    EVENTKV("target_amount", target_amount) \
    EVENTKV("base_weight", base_weight) \
    EVENTKV("target_weight", target_weight) \
    EVENTKV("to_connector_max_fee", to_connector_max_fee) \
    EVENTKV("from_connector_max_fee", from_connector_max_fee) \
    EVENTKV("to_connector_fee", to_connector_fee) \
    EVENTKVL("from_connector_fee", from_connector_fee) \
    END_EVENT()

CONTRACT BancorConverter : public eosio::contract {
    using contract::contract;
    public:

        TABLE connector_t {
            name     contract;
            asset    currency;
            uint64_t weight;
            bool     enabled;
            uint64_t fee;
            name     fee_account;
            uint64_t max_fee;        
            uint64_t primary_key() const { return currency.symbol.code().raw(); }
        };
    
        TABLE cstate_t {
            name    manager;
            name    smart_contract;
            asset   smart_currency;
            bool    smart_enabled;
            bool    enabled;
            name    network;
            bool    verify_ram;
            uint64_t primary_key() const { return manager.value; }
        };

        typedef eosio::multi_index<"cstate"_n, cstate_t> converters;
        typedef eosio::multi_index<"connector"_n, connector_t> connectors;

        void transfer(name from, name to, asset quantity, string memo);

        ACTION create(name smart_contract,
                      asset smart_currency,
                      bool  smart_enabled,
                      bool  enabled,
                      name  network,
                      bool  verify_ram);
        
        ACTION setconnector(name contract,
                            asset    currency,
                            uint64_t weight,
                            bool     enabled,
                            uint64_t fee,
                            name     fee_account,
                            uint64_t max_fee );
    private:
        void convert(name from, eosio::asset quantity, std::string memo, name code);
        const connector_t& lookup_connector(uint64_t name, cstate_t state );
};
