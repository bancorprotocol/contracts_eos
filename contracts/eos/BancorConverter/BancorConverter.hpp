#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/symbol.hpp>
#include <eosiolib/singleton.hpp>
#include "../Common/common.hpp"

using namespace eosio;
using std::string;
using std::vector;

#define EMIT_CONVERSION_EVENT(memo, current_base_balance, current_target_balance, current_smart_supply, base_amount, smart_tokens, target_amount, base_ratio, target_ratio, fee_amount) \
    START_EVENT("conversion", "1.1") \
    EVENTKV("memo", memo) \
    EVENTKV("current_base_balance", current_base_balance) \
    EVENTKV("current_target_balance", current_target_balance) \
    EVENTKV("current_smart_supply", current_smart_supply) \
    EVENTKV("base_amount", base_amount) \
    EVENTKV("smart_tokens", smart_tokens) \
    EVENTKV("target_amount", target_amount) \
    EVENTKV("base_ratio", base_ratio) \
    EVENTKV("target_ratio", target_ratio) \
    EVENTKVL("fee_amount", fee_amount) \
    END_EVENT()

CONTRACT BancorConverter : public eosio::contract {
    using contract::contract;
    public:

        TABLE settingstype {
            name     smart_contract;
            asset    smart_currency;
            bool     smart_enabled;
            bool     enabled;
            name     network;
            bool     verify_ram;
            uint64_t max_fee;
            uint64_t fee;
            EOSLIB_SERIALIZE(settingstype, (smart_contract)(smart_currency)(smart_enabled)(enabled)(network)(verify_ram)(max_fee)(fee))
        };

        TABLE reserve_t {
            name     contract;
            asset    currency;
            uint64_t ratio;
            bool     p_enabled;
            uint64_t primary_key() const { return currency.symbol.code().raw(); }
        };

        typedef eosio::singleton<"settings"_n, settingstype> settings;
        typedef eosio::multi_index<"reserves"_n, reserve_t> reserves;

        void transfer(name from, name to, asset quantity, string memo);

        ACTION create(name smart_contract,
                      asset smart_currency,
                      bool  smart_enabled,
                      bool  enabled,
                      name  network,
                      bool  verify_ram,
                      uint64_t max_fee,
                      uint64_t fee);
        
        ACTION setreserve(name contract,
                          asset    currency,
                          uint64_t ratio,
                          bool     p_enabled);

    private:
        void convert(name from, eosio::asset quantity, std::string memo, name code);
        const reserve_t& lookup_reserve(uint64_t name, const settingstype& settings);
};
