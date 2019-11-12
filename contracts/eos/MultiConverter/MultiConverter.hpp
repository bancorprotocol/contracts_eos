
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

/**
 * @defgroup MultiConverter MultiConverter
 * @brief Bancor MultiConverter
 * @details The Bancor converter allows conversions between a smart token and tokens
 * that are defined as its reserves and between the different reserves directly.
 * @{
*/

/// triggered when a conversion between two tokens occurs
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

/// triggered after a conversion with new tokens price data
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

/// triggered after a fee update occurs for a converter
#define EMIT_CONVERSION_FEE_UPDATE_EVENT(converter_currency_symbol, prev_fee, new_fee) { \
    START_EVENT("conversion_fee_update", "1.2") \
    EVENTKV("converter_currency_symbol", converter_currency_symbol) \
    EVENTKV("prev_fee", prev_fee) \
    EVENTKVL("new_fee", new_fee) \
    END_EVENT() \
}

/*! \cond DOCS_EXCLUDE */
CONTRACT MultiConverter : public eosio::contract { /*! \endcond */
    public:
        using contract::contract;

        /** 
         * @defgroup MultiConverter_Settings_Table Settings Table
         * @brief This table stores the global settings affecting all the converters in this contract
         * @details Both SCOPE and PRIMARY KEY are `_self`, so this table is effectively a singleton
         * @{
         *//*! \cond DOCS_EXCLUDE */
            TABLE settings_t { /*! \endcond */
                /**
                 * @brief toggle boolean to enable/disable all the converters in this contract
                 */
                bool operational;

                /**
                 * @brief maximum conversion fee for converters in this contract
                 */
                uint64_t max_fee;

                /**
                 * @brief account name of contract for relay tokens
                 */
                name multi_token;
                
                /**
                 * @brief account name of contract for voting and staking
                 */
                name staking;
        
                /*! \cond DOCS_EXCLUDE */
                uint64_t primary_key() const { return "settings"_n.value; } 
                /*! \endcond */

            }; /** @}*/

        /** 
         * @defgroup MultiConverter_Reserves_Table Reserves Table
         * @brief This table stores the reserve balances and related information for the reserves of every converter in this contract
         * @details SCOPE of this table is the converters' smart token symbol's `code().raw()` values
         * @{
         *//*! \cond DOCS_EXCLUDE */
            TABLE reserve_t { /*! \endcond */
                /**
                 * @brief name of the account storing the token contract for this reserve's token
                 */
                name contract;

                /**
                 * @brief reserve ratio relative to the other reserves 
                 */
                uint64_t ratio;

                /**
                 * @brief toggle boolean to enable/disable conversions through this reserve
                 */
                bool sale_enabled;

                /**
                 * @brief amount in the reserve
                 * @details PRIMARY KEY for this table is `balance.symbol.code().raw()`
                 */
                asset balance;

                /*! \cond DOCS_EXCLUDE */
                uint64_t primary_key() const { return balance.symbol.code().raw(); }
                /*! \endcond */

            }; /** @}*/
        
        /** 
         * @defgroup Converters_Table Converters Table
         * @brief This table stores the key information about all converters in this contract
         * @details SCOPE of this table is the converters' smart token symbol's `code().raw()` values
         * @{
         *//*! \cond DOCS_EXCLUDE */
            TABLE converter_t { /*! \endcond */
                /**
                 * @brief symbol of the smart token -- representing a share in the reserves of this converter
                 * @details PRIMARY KEY for this table is `currency.code().raw()`
                 */
                symbol currency; 

                /**
                 * @brief creator of the converter
                 */
                name owner;

                /**
                 * @brief toggle boolean to enable/disable this converter
                 */
                bool enabled;
 
                /**
                 * @brief Has this converter launched (ever been enabled)
                 */
                bool launched;

                /**                
                 * @brief toggle boolean to enable/disable this staking and voting for this converter
                 */
                bool stake_enabled;

                /**
                 * @brief conversion fee for this converter, applied on every hop
                 */
                uint64_t fee;

                /*! \cond DOCS_EXCLUDE */
                uint64_t primary_key() const { return currency.code().raw(); }
                /*! \endcond */                

            }; /** @}*/

        /**
         * @defgroup Accounts_Table Accounts Table
         * @brief This table stores "temporary balances" that are transfered in by liquidity providers before they can get added to their respective reserves
         * @details SCOPE of this table is the `name.value` of the liquidity provider, owner of the `quantity`
         * @{
         *//*! \cond DOCS_EXCLUDE */
            TABLE account_t { /*! \endcond */ 
                /**
                 * @brief symbol of the smart token (a way to reference converters) this balance pertains to
                 */
                symbol_code symbl; 

                /**
                 * @brief balance in the reserve currency
                 */
                asset quantity; 

                /**
                 * @brief PRIMARY KEY for this table, unused dummy variable
                 */
                uint64_t id; 

                /**
                 * @brief SECONDARY KEY of this table based on the symbol of the temporary reserve balance for a particular converter 
                 * @details uint128_t { balance.symbol.code().raw() } << 64 ) | currency.raw()
                 */
                uint128_t by_cnvrt()   const { return _by_cnvrt(quantity, symbl); }

                /*! \cond DOCS_EXCLUDE */
                uint64_t    primary_key() const { return id; }
                bool        is_empty()    const { return quantity.amount == 0; }
                /*! \endcond */

            }; /** @}*/

        /**
         * @brief initializes a new converter
         * @param owner - the converter creator
         * @param initial_supply - initial supply of the smart token governed by the converter
         * @param maximum_supply - maximum supply of the smart token governed by the converter
         */
        ACTION create(name owner, asset initial_supply, asset maximum_supply);

        /**
         * @brief deletes a converter with empty reserves
         * @param converter_currency_code - the currency code of the currency governed by the converter
         */
        ACTION close(symbol_code converter_currency_code);

        /**
         * @brief creates the multi-converter settings, can only be called by multi-converter owner
         * @param multi_token - may only set multi-token contract once
         */
        ACTION setmultitokn(name multi_token); 
        
        // the 3 actions below modify multi-converter settings after the multi-token contract has been set

        /**
         * @brief may only set staking/voting contract for this multi-converter once
         * @param staking - name of staking/voting contract
         */
        ACTION setstaking(name staking);

        /**
         * @brief modify maxfee in this multi-converter's settings
         * @param maxfee - maximum fee for all converters in this multi-converter
         */
        ACTION setmaxfee(uint64_t maxfee);

        /**
         * @brief modify enabled in this multi-converter's settings
         * @param enabled - false will override individual enabled flags for all converters
         */
        ACTION setenabled(bool enabled);
        
        // the 4 actions below updates the converter settings, can only be called by the converter owner after creation

        /**
         * @brief change converter's owner
         * @param currency - the currency symbol governed by the converter
         * @param new_owner - converter's new owner
         */
        ACTION updateowner(symbol_code currency, name new_owner);                 
        
        /**
         * @brief updates the converter fee
         * @param currency - the currency symbol governed by the converter
         * @param fee - the new fee % for this converter, must be lower than the maximum fee, 0-1000000
         */
        ACTION updatefee(symbol_code currency, uint64_t fee);
        
        /**
         * @brief flag indicating if conversions are enabled, false if not
         * @param currency - the currency symbol governed by the converter
         * @param enabled - true if conversions for this symbol are enabled
         */
        ACTION enablecnvrt(symbol_code currency, bool enabled); 
        
        /**
         * @brief flag indicating if the smart token can be staked, false if not
         * @param currency - the currency symbol governed by the converter
         * @param enabled - true if staking/voting for this symbol are enabled
         */
        ACTION enablestake(symbol_code currency, bool enabled);

        /**
         * @brief initializes a new reserve in the converter
         * @details can also be used to update an existing reserve, can only be called by the contract account
         * @param converter_currency_code - the currency code of the currency governed by the converter
         * @param currency - reserve token currency symbol
         * @param contract - reserve token contract name
         * @param sale_enabled - true if selling is enabled with the reserve, false if not
         * @param ratio - reserve ratio, percentage, 0-1000000
         */
        ACTION setreserve(symbol_code converter_currency_code, symbol currency, name contract, bool sale_enabled, uint64_t ratio);

        /**
         * @brief deletes an empty reserve in the converter
         * @param converter - the currency code of the smart token governed by the converter
         * @param currency - reserve token currency code
         */
        ACTION delreserve(symbol_code converter, symbol_code currency);

        /**
         * @brief called by liquidity providers withdrawing "temporary balances" before `fund`ing them into the reserve
         * @param sender - sender of the quantity
         * @param quantity - amount to decrease the supply by (in the smart token)
         * @param converter_currency_code - the currency code of the currency governed by the converter
        */
        ACTION withdraw(name sender, asset quantity, symbol_code converter_currency_code);
        
        /**
         * @brief buys smart tokens with all connector tokens using the same percentage
         * @details i.e. if the caller increases the supply by 10%, it will cost an amount equal to
         * 10% of each connector token balance
         * can only be called if the total ratio is exactly 100% and while conversions are enabled
         * @param  sender - sender of the quantity
         * @param quantity - amount to increase the supply by (in the smart token)
        */
        ACTION fund(name sender, asset quantity); 
        
        /**
         * @brief transfer intercepts with standard transfer args
         * @details `memo` containing a keyword following a semicolon at the end of the conversion path indicates special kind of transfer which otherwise would be interpreted as a standard conversion:
         * - e.g. transferring smart tokens with keyword "liquidate", or
         * - transferring reserve tokens with keyword "fund"
         * @param from - the sender of the transfer
         * @param to - the receiver of the transfer
         * @param quantity - the quantity for the transfer
         * @param memo - the memo for the transfer
         */
        //[[eosio::on_notify("*::transfer")]]
        void on_transfer(name from, name to, asset quantity, string memo);
        //using transfer_action = action_wrapper<name("transfer"), &MultiConverter::on_transfer>;
        
        /*! \cond DOCS_EXCLUDE */
        typedef eosio::multi_index<"settings"_n, settings_t> settings;
        typedef eosio::multi_index<"converters"_n, converter_t> converters;
        typedef eosio::multi_index<"reserves"_n, reserve_t> reserves;
        typedef eosio::multi_index<"accounts"_n, account_t,         
                        indexed_by<"bycnvrt"_n, 
                            const_mem_fun <account_t, uint128_t, 
                            &account_t::by_cnvrt >>> accounts;
        /*! \endcond */ 

    private: 
        void convert(name from, asset quantity, string memo, name code);

        const reserve_t& get_reserve(uint64_t name, const converter_t& converter);

        void mod_reserve_balance(symbol converter_currency, asset value);
        void mod_account_balance(name sender, symbol_code converter_currency_code, asset quantity);
        void mod_balances(name sender, asset quantity, symbol_code converter_currency_code, name code);
        
        /**
         * @brief sells the token for all connector tokens using the same percentage
         * @details i.e. if the holder sells 10% of the supply, they will receive 10% of each
         * connector token balance in return
         * can only be called if the max total weight is exactly 100%
         * note that the function can also be called if conversions are disabled
        */
        void liquidate(name sender, asset quantity); // quantity to decrease the supply by (in the smart token)

        double calculate_fee(double amount, uint64_t fee, uint8_t magnitude);
        
        asset get_supply(name contract, symbol_code sym);

        void verify_min_return(asset quantity, string min_return);
        void verify_entry(name account, name currency_contract, symbol currency);

        double calculate_purchase_return(double balance, double deposit_amount, double supply, int64_t ratio);
        double calculate_sale_return(double balance, double sell_amount, double supply, int64_t ratio);
        double quick_convert(double balance, double in, double toBalance);

        float stof(const char* s);

        static uint128_t _by_cnvrt( asset balance, symbol_code converter_currency_code ) {
           return ( uint128_t{ balance.symbol.code().raw() } << 64 ) | converter_currency_code.raw();
        } 
        constexpr static double MAX_RATIO = 1000000.0;
        constexpr static double MAX_FEE = 1000000.0;

}; /** @}*/
