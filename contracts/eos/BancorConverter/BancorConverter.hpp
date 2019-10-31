
/**
 *  @file
 *  @copyright defined in ../../../LICENSE
 */
#pragma once

#include <eosio/eosio.hpp>
#include <eosio/transaction.hpp>
#include <eosio/asset.hpp>
#include <eosio/symbol.hpp>

using namespace eosio;
using namespace std;

/// triggered when a conversion between two tokens occurs
#define EMIT_CONVERSION_EVENT(memo, from_contract, from_symbol, to_contract, to_symbol, from_amount, to_amount, fee_amount) { \
    START_EVENT("conversion", "1.3") \
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

/// triggered after a conversion with new tokens price data
#define EMIT_PRICE_DATA_EVENT(smart_supply, reserve_contract, reserve_symbol, reserve_balance, reserve_ratio) { \
    START_EVENT("price_data", "1.4") \
    EVENTKV("smart_supply", smart_supply) \
    EVENTKV("reserve_contract", reserve_contract) \
    EVENTKV("reserve_symbol", reserve_symbol) \
    EVENTKV("reserve_balance", reserve_balance) \
    EVENTKVL("reserve_ratio", reserve_ratio) \
    END_EVENT() \
}

/// triggered when the conversion fee is updated
#define EMIT_CONVERSION_FEE_UPDATE_EVENT(prev_fee, new_fee) { \
    START_EVENT("conversion_fee_update", "1.1") \
    EVENTKV("prev_fee", prev_fee) \
    EVENTKVL("new_fee", new_fee) \
    END_EVENT() \
}

/**
 * @defgroup bancorconverter BancorConverter
 * @ingroup bancorcontracts
 * @brief Bancor Converter
 * @details The Bancor converter allows conversions between a smart token and tokens
 * that are defined as its reserves and between the different reserves directly.
 * Reserve balance can be virtual, meaning that the calculations are based on
 * the virtual balance instead of relying on the actual reserve balance.
 * This is a security mechanism that prevents the need to keep a very large
 * (and valuable) balance in a single contract.
 * @{
*/
CONTRACT BancorConverter : public eosio::contract {
    public:
        using contract::contract;
        
        TABLE settings_t {
            name     smart_contract;
            asset    smart_currency;
            bool     smart_enabled;
            bool     enabled;
            name     network;
            uint64_t max_fee;
            uint64_t fee;		
            
            uint64_t primary_key() const { return "settings"_n.value; }
        };

        TABLE reserve_t {
            name     contract;
            asset    currency;
            uint64_t ratio;
            bool     sale_enabled;
            uint64_t primary_key() const { return currency.symbol.code().raw(); }
        };

        typedef eosio::multi_index<"settings"_n, settings_t> settings;
        typedef eosio::multi_index<"reserves"_n, reserve_t> reserves;

        /**
         * @brief initializes the converter settings
         * @details can only be called once, by the contract account
         * @param smart_contract - contract name of the smart token governed by the converter
         * @param smart_currency - currency of the smart token governed by the converter
         * @param smart_enabled - true if the smart token can be converted to/from, false if not
         * @param enabled - true if conversions are enabled, false if not
         * @param network - bancor network contract name
         * @param max_fee - maximum conversion fee percentage, 0-30000, 4-pt precision a la eosio.asset
         * @param fee - conversion fee percentage, must be lower than the maximum fee, same precision
         */ 
        ACTION init(name smart_contract, asset smart_currency, bool smart_enabled, bool enabled, name network, uint64_t max_fee, uint64_t fee);

        /**
         * @brief updates the converter settings
         * @details can only be called by the contract account
         * @param smart_enabled - true if the smart token can be converted to/from, false if not
         * @param enabled - true if conversions are enabled, false if not
         * @param fee - conversion fee percentage, must be lower than the maximum fee, same precision
         */
        ACTION update(bool smart_enabled, bool enabled, uint64_t fee);
        
        /**
         * @brief initializes a new reserve in the converter
         * @details can also be used to update an existing reserve, can only be called by the contract account
         * @param contract - reserve token contract name
         * @param currency - reserve token currency symbol
         * @param ratio - reserve ratio, percentage, 0-1000000, precision a la max_fee
         * @param sale_enabled - true if purchases are enabled with the reserve, false if not
         */ 
        ACTION setreserve(name contract, symbol currency, uint64_t ratio, bool sale_enabled);

        /**
         * @brief deletes an empty reserve
         * @param currency - reserve token currency symbol
         */
        ACTION delreserve(symbol_code currency);

        /**
         * @brief transfer intercepts
         * @details memo is in csv format, values -
         * version          version number, currently 1
         * path             conversion path, see conversion path in the BancorNetwork contract
         * minimum return   conversion minimum return amount, the conversion will fail if the amount returned is lower than the given amount
         * target account   account to receive the conversion return
         */
        [[eosio::on_notify("*::transfer")]]
        void on_transfer(name from, name to, asset quantity, string memo);
        using transfer_action = action_wrapper<name("transfer"), &BancorConverter::on_transfer>;

    private:
        void convert(name from, eosio::asset quantity, std::string memo, name code);
        const reserve_t& get_reserve(uint64_t name, const settings_t& settings);

        asset get_balance(name contract, name owner, symbol_code sym);
        uint64_t get_balance_amount(name contract, name owner, symbol_code sym);
        asset get_supply(name contract, symbol_code sym);

        double calculate_purchase_return(double balance, double deposit_amount, double supply, int64_t ratio);
        double calculate_sale_return(double balance, double sell_amount, double supply, int64_t ratio);
        double quick_convert(double balance, double in, double toBalance);
};
/** @}*/ // end of @defgroup bancorconverter BancorConverter
