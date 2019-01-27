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

// events
// triggered when a conversion between two tokens occurs
#define EMIT_CONVERSION_EVENT(memo, from_contract, from_symbol, to_contract, to_symbol, from_amount, to_amount, fee_amount) \
    START_EVENT("conversion", "1.2") \
    EVENTKV("memo", memo) \
    EVENTKV("from_contract", from_contract) \
    EVENTKV("from_symbol", from_symbol) \
    EVENTKV("to_contract", to_contract) \
    EVENTKV("to_symbol", to_symbol) \
    EVENTKV("amount", from_amount) \
    EVENTKV("return", to_amount) \
    EVENTKVL("conversion_fee", fee_amount) \
    END_EVENT()

// triggered after a conversion with new tokens price data
#define EMIT_PRICE_DATA_EVENT(smart_supply, reserve_contract, reserve_symbol, reserve_balance, reserve_ratio) \
    START_EVENT("price_data", "1.2") \
    EVENTKV("smart_supply", smart_supply) \
    EVENTKV("reserve_contract", reserve_contract) \
    EVENTKV("reserve_symbol", reserve_symbol) \
    EVENTKV("reserve_balance", reserve_balance) \
    EVENTKVL("reserve_ratio", reserve_ratio) \
    END_EVENT()

/*
    Bancor Converter

    The Bancor converter allows conversions between a smart token and tokens
    that are defined as its reserves and between the different reserves directly.

    Reserve balance can be virtual, meaning that the calculations are based on
    the virtual balance instead of relying on the actual reserve balance.
    This is a security mechanism that prevents the need to keep a very large
    (and valuable) balance in a single contract.
*/
CONTRACT BancorConverter : public eosio::contract {
    using contract::contract;
    public:

        TABLE settings_t {
            name     smart_contract;
            asset    smart_currency;
            bool     smart_enabled;
            bool     enabled;
            name     network;
            bool     require_balance;
            uint64_t max_fee;
            uint64_t fee;
            EOSLIB_SERIALIZE(settings_t, (smart_contract)(smart_currency)(smart_enabled)(enabled)(network)(require_balance)(max_fee)(fee))
        };

        TABLE reserve_t {
            name     contract;
            asset    currency;
            uint64_t ratio;
            bool     p_enabled;
            uint64_t primary_key() const { return currency.symbol.code().raw(); }
        };

        typedef eosio::singleton<"settings"_n, settings_t> settings;
        typedef eosio::multi_index<"settings"_n, settings_t> dummy_for_abi; // hack until abi generator generates correct name
        typedef eosio::multi_index<"reserves"_n, reserve_t> reserves;

        // initializes the converter settings
        // can only be called once, by the contract account
        ACTION init(name smart_contract,    // contract name of the smart token governed by the converter
                    asset smart_currency,   // currency of the smart token governed by the converter
                    bool  smart_enabled,    // true if the smart token can be converted to/from, false if not
                    bool  enabled,          // true if conversions are enabled, false if not
                    name  network,          // bancor network contract name
                    bool  require_balance,  // true if conversions that require creating new balance for the calling account should fail, false if not
                    uint64_t max_fee,       // maximum conversion fee percentage, 0-1000
                    uint64_t fee);          // conversion fee percentage, must be lower than the maximum fee, 0-1000

        // updates the converter settings
        // can only be called by the contract account
        ACTION update(bool smart_enabled,    // true if the smart token can be converted to/from, false if not
                      bool enabled,          // true if conversions are enabled, false if not
                      bool require_balance,  // true if conversions that require creating new balance for the calling account should fail, false if not
                      uint64_t fee);         // conversion fee percentage, must be lower than the maximum fee, 0-1000
        
        // initializes a new reserve in the converter
        // can also be used to update an existing reserve, can only be called by the contract account
        ACTION setreserve(name contract,        // reserve token contract name
                          asset    currency,    // reserve token currency
                          uint64_t ratio,       // reserve ratio, percentage, 0-1000
                          bool     p_enabled);  // true if purchases are enabled with the reserve, false if not

        // transfer intercepts
        // memo is in csv format, values -
        // version          version number, currently 1
        // path             conversion path, see conversion path in the BancorNetwork contract
        // minimum return   conversion minimum return amount, the conversion will fail if the amount returned is lower than the given amount
        // target account   account to receive the conversion return
        void transfer(name from, name to, asset quantity, string memo);

    private:
        void convert(name from, eosio::asset quantity, std::string memo, name code);
        const reserve_t& get_reserve(uint64_t name, const settings_t& settings);

        asset get_balance(name contract, name owner, symbol_code sym);
        uint64_t get_balance_amount(name contract, name owner, symbol_code sym);
        asset get_supply(name contract, symbol_code sym);

        void verify_entry(name account, name currency_contact, eosio::asset currency);
        void verify_min_return(eosio::asset quantity, std::string min_return);

        double calculate_purchase_return(double balance, double deposit_amount, double supply, int64_t ratio);
        double calculate_sale_return(double balance, double sell_amount, double supply, int64_t ratio);
        double quick_convert(double balance, double in, double toBalance);

        float stof(const char* s);
};
