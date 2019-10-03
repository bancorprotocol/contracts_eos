#pragma once

#include <eosio/eosio.hpp>
#include <eosio/transaction.hpp>
#include <eosio/asset.hpp>
#include <eosio/symbol.hpp>
#include "../Common/common.hpp"

using namespace eosio;
using namespace std;

// events
// triggered when a conversion between two tokens occurs
#define EMIT_CONVERSION_EVENT(converter_currency_symbol, memo, from_contract, from_symbol, to_contract, to_symbol, from_amount, to_amount, fee_amount){ \
    START_EVENT("conversion", "1.4") \
    EVENTKV("converter_currency_symbol", converter_currency_symbol) \
    EVENTKV("memo", memo) \
    EVENTKV("from_contract", from_contract) \
    EVENTKV("from_symbol", from_symbol) \
    EVENTKV("to_contract", to_contract) \
    EVENTKV("to_symbol", to_symbol) \
    EVENTKV("amount", from_amount) \
    EVENTKV("return", to_amount) \
    EVENTKVL("conversion_fee", fee_amount) \
    END_EVENT() \
}

// triggered after a conversion with new tokens price data
#define EMIT_PRICE_DATA_EVENT(converter_currency_symbol, smart_supply, reserve_contract, reserve_symbol, reserve_balance, reserve_ratio) { \
    START_EVENT("price_data", "1.5") \
    EVENTKV("converter_currency_symbol", converter_currency_symbol) \
    EVENTKV("smart_supply", smart_supply) \
    EVENTKV("reserve_contract", reserve_contract) \
    EVENTKV("reserve_symbol", reserve_symbol) \
    EVENTKV("reserve_balance", reserve_balance) \
    EVENTKVL("reserve_ratio", reserve_ratio) \
    END_EVENT() \
}

#define EMIT_CONVERSION_FEE_UPDATE_EVENT(converter_currency_symbol, prev_fee, new_fee) { \
    START_EVENT("conversion_fee_update", "1.2") \
    EVENTKV("converter_currency_symbol", converter_currency_symbol) \
    EVENTKV("prev_fee", prev_fee) \
    EVENTKVL("new_fee", new_fee) \
    END_EVENT() \
}

/*
    Bancor Converter

    The Bancor converter allows conversions between a smart token and tokens
    that are defined as its reserves and between the different reserves directly.

    Reserve balance can be virtual, meaning that the calculations are based on
    the virtual balance instead of relying on the actual reserve balance.
    This is a security mechanism that prevents the need to keep a very large
    (and valuable) balance in a single contract.
*/
CONTRACT MultiConverter : public eosio::contract {
    public:
        using contract::contract;

        TABLE settings_t {
            bool     enabled = false;
            uint64_t max_fee = MAX_FEE;
            name     multi_token; // account name of contract for relay tokens
            name     staking;     // account name of contract for voting and staking
            uint64_t primary_key() const { return "settings"_n.value; }
            
            EOSLIB_SERIALIZE(settings_t, (enabled)(max_fee)(multi_token)(staking))
        };

        TABLE reserve_t {
            name     contract;
            uint64_t ratio;
            bool     sale_enabled;
            asset    balance;
            uint64_t primary_key() const { return balance.symbol.code().raw(); }
        };
        
        TABLE converter_t {
            symbol   currency;
            name     owner;
            bool     enabled = false;
            bool     launched = false;
            bool     stake_enabled = false;
            uint64_t fee = 0;
            uint64_t primary_key() const { return currency.code().raw(); }
        };

        TABLE account_t {
            symbol_code currency;
            asset       balance;
            uint64_t    id; // dummy just for primary_index
            uint64_t    primary_key() const { return id; }
            bool        is_empty()    const { return balance.amount == 0; }
            uint128_t   by_cnvrt()   const { return _by_cnvrt(balance, currency); }
        };

        typedef eosio::multi_index<"settings"_n, settings_t> settings;
        typedef eosio::multi_index<"converters"_n, converter_t> converters;
        typedef eosio::multi_index<"reserves"_n, reserve_t> reserves;
        typedef eosio::multi_index<"accounts"_n, account_t,         
                        indexed_by<"bycnvrt"_n, 
                            const_mem_fun <account_t, uint128_t, 
                            &account_t::by_cnvrt >>> accounts;

        // initializes a new converter
        ACTION create(name     owner,              // the converter creator
                      asset    initial_supply,     // initial supply of the smart token governed by the converter
                      asset    maximum_supply);    // maximum supply of the smart token governed by the converter

        // creates the multi-converter settings, can only be called by multi-converter owner
        ACTION setmultitokn(name multi_token); // may only set multi-token contract once
        // the 3 actions below modify multi-converter settings after the multi-token contract has been set
        ACTION setstaking(name staking); // may only set staking/voting contract once
        ACTION setmaxfee(uint64_t maxfee);
        ACTION setenabled(bool enabled);
        
        // the 4 actions below updates the converter settings, can only be called by the converter owner after creation
        ACTION updateowner(symbol_code currency,    // the currency governed by the converter
                           name        owner);      // converter's new owner                 
        // updates the converter fee, can be called by staking contract or converter owner is staking disabled
        ACTION updatefee(symbol_code currency,    // the currency governed by the converter
                         uint64_t    fee);        // conversion fee percentage, must be lower than the maximum fee, 0-1000000
        
        // toggle if conversions are enabled, false if not
        ACTION enablecnvrt(symbol_code currency, bool enabled);  // the currency governed by the converter
        // toggle if the smart token can be staked, false if not                      
        ACTION enablestake(symbol_code currency, bool enabled);  // the currency governed by the converter

        // initializes a new reserve in the converter
        // can also be used to update an existing reserve, can only be called by the contract account
        ACTION setreserve(symbol_code converter_currency_code,    // the currency code of the currency governed by the converter
                          asset       currency,                   // reserve token currency
                          name        contract,                   // reserve token contract name
                          bool        sale_enabled,                  // true if purchases are enabled with the reserve, false if not
                          uint64_t    ratio);                     // reserve ratio, percentage, 0-1000000

        // called by withdrawing liquidity providers before converter is enabled for the first time
        ACTION withdraw(name owner, asset quantity, symbol_code converter_currency_code);
        
        /*
         * 
         * buys the token with all connector tokens using the same percentage
         * i.e. if the caller increases the supply by 10%, it will cost an amount equal to
         * 10% of each connector token balance
         * can only be called if the max total weight is exactly 100% and while conversions are enabled
        */
        ACTION fund(name owner, asset quantity); // amount to increase the supply by (in the smart token)
          
        // transfer intercepts
        // containing a keyword followed by semicolon indicates special kind of transfer:
        // e.g. transferring smart token with keyword "liquidate",
        // transferring reserve tokens with keyword "fund" after converter launched,
        // or using "setup" keyword before converter launched
        
        // regular conversion memo is in csv format, values:
        // version          version number, currently 1
        // path             conversion path, see conversion path in the BancorNetwork contract
        // minimum return   conversion minimum return amount, the conversion will fail if the amount returned is lower than the given amount
        // target account   account to receive the conversion return
        void transfer(name from, name to, asset quantity, string memo);

    private:
        void convert(name from, asset quantity, string memo, name code);
        void _setreserve(symbol_code converter_currency_code, name owner, symbol currency, name contract, bool sale_enabled, uint64_t ratio, uint64_t smart_token_supply);

        const reserve_t& get_reserve(uint64_t name, const converter_t& converter);

        void mod_reserve_balance(symbol converter_currency, asset value);
        void mod_account_balance(name owner, symbol_code converter_currency_code, asset quantity);
        void modreserve(name owner, asset quantity, symbol_code converter_currency_code, name code, bool setup);
        
        /*
         * sells the token for all connector tokens using the same percentage
         * i.e. if the holder sells 10% of the supply, they will receive 10% of each
         * connector token balance in return
         * can only be called if the max total weight is exactly 100%
         * note that the function can also be called if conversions are disabled
        */
        void liquidate(name owner, asset quantity); // amount to decrease the supply by (in the smart token)

        double calculate_fee(double amount, uint64_t fee, uint8_t magnitude);

        asset get_supply(name contract, symbol_code sym);

        void verify_min_return(asset quantity, string min_return);
        void verify_entry(name account, name currency_contact, symbol currency);

        double calculate_purchase_return(double balance, double deposit_amount, double supply, int64_t ratio);
        double calculate_sale_return(double balance, double sell_amount, double supply, int64_t ratio);
        double quick_convert(double balance, double in, double toBalance);

        float stof(const char* s);

        static uint128_t _by_cnvrt( asset balance, symbol_code converter_currency_code ) {
            return ( uint128_t{ balance.symbol.code().raw() } << 64 ) | converter_currency_code.raw();
        }
        constexpr static double MAX_RATIO = 1000000.0;
        constexpr static double MAX_FEE = 1000000.0;
};
