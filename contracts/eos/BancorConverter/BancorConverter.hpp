
/**
 *  @file
 *  @copyright defined in ../../../LICENSE
 */
#pragma once

#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/transaction.hpp>
#include <eosio/asset.hpp>
#include <eosio/symbol.hpp>

#include "../Common/common.hpp"

using namespace eosio;
using namespace std;

/**
 * @defgroup BancorConverter BancorConverter
 * @brief Bancor BancorConverter
 * @details The Bancor converter allows conversions between a smart token and tokens
 * that are defined as its reserves and between the different reserves directly.
 * @{
*/

/*! \cond DOCS_EXCLUDE */
class [[eosio::contract]] BancorConverter : public contract { /*! \endcond */
    public:
        using contract::contract;

        /**
         * @defgroup BancorConverter_Settings_Table Settings Table
         * @brief This table stores the global settings affecting all the converters in this contract
         * @details Both SCOPE and PRIMARY KEY are `_self`, so this table is effectively a singleton
         * @{
         *//*! \cond DOCS_EXCLUDE */
            struct [[eosio::table("settings")]] settings_t { /*! \endcond */
                /**
                 * @brief maximum conversion fee for converters in this contract
                 */
                uint64_t max_fee;

                /**
                 * @brief account name of contract for relay tokens
                 */
                name multi_token;

                /**
                 * @brief account name of the bancor network contract
                 */
                name network;

                /**
                 * @brief account name of contract for voting and staking
                 */
                name staking;
            }; /** @}*/

        /**
         * @defgroup BancorConverter_Reserves_Table Reserves Table
         * @brief This table stores the reserve balances and related information for the reserves of every converter in this contract
         * @details SCOPE of this table is the converters' smart token symbol's `code().raw()` values
         * @{
         *//*! \cond DOCS_EXCLUDE */
            struct [[eosio::table("reserves")]] reserve_t { /*! \endcond */
                /**
                 * @brief name of the account storing the token contract for this reserve's token
                 */
                name contract;

                /**
                 * @brief reserve ratio relative to the other reserves
                 */
                uint64_t ratio;

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
         * @defgroup BancorConverter_Reserves_Table Reserves Table
         * @brief This table stores the reserve balances and related information for the reserves of every converter in this contract
         * @details SCOPE of this table is the converters' smart token symbol's `code().raw()` values
         * @{
         *//*! \cond DOCS_EXCLUDE */
            struct [[eosio::table("converter.v2")]] converter_v2_t {
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
                 * @brief conversion fee for this converter, applied on every hop
                 */
                uint64_t fee;

                /**
                 * @brief reserve weights relative to the other reserves
                 * @example
                 * {
                 *   "key: "BNT",
                 *   "value": 500000
                 * }
                 */
                map<symbol_code, uint64_t> reserve_weights;

                /**
                 * @brief balances in each reserve
                 * @example
                 * {
                 *   "key: "BNT",
                 *   "value": "10000.0000 BNT"
                 * }
                 */
                map<symbol_code, extended_asset> reserve_balances;

                /**
                 * @brief [optional] protocol features for converter
                 * @example
                 * {
                 *   "key: "stake",
                 *   "value": true
                 * }
                 */
                map<name, bool> protocol_features;

                /**
                 * @brief [optional] additional metadata for converter
                 * @example
                 * {
                 *   "key: "website",
                 *   "value": "https://mywebsite.com"
                 * }
                 */
                map<name, string> metadata_json;

                /*! \cond DOCS_EXCLUDE */
                uint64_t primary_key() const { return currency.code().raw(); }
                /*! \endcond */

            }; /** @}*/

        /**
         * @defgroup BancorConverter_Converters_Table Converters Table
         * @brief This table stores the key information about all converters in this contract
         * @details SCOPE of this table is the converters' smart token symbol's `code().raw()` values
         * @{
         *//*! \cond DOCS_EXCLUDE */
            struct [[eosio::table("converters")]] converter_t { /*! \endcond */
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
         * @defgroup BancorConverter_Accounts_Table Accounts Table
         * @brief This table stores "temporary balances" that are transfered in by liquidity providers before they can get added to their respective reserves
         * @details SCOPE of this table is the `name.value` of the liquidity provider, owner of the `quantity`
         * @{
         *//*! \cond DOCS_EXCLUDE */
            struct [[eosio::table("account")]] account_t { /*! \endcond */
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
        [[eosio::action]]
        void create(name owner, symbol_code token_code, double initial_supply);

        /**
         * @brief deletes a converter with empty reserves
         * @param converter_currency_code - the currency code of the currency governed by the converter
         */
        [[eosio::action]]
        void delconverter(symbol_code converter_currency_code);

        /**
         * @brief sets the bancor settings
         * @param multi_token - may only set multi-token contract once
         * @param staking - name of staking/voting contract
         * @param maxfee - maximum fee for all converters in this multi-converter
         * @param network - bancor network contract account
         * @example
         *
         * cleos push action bancorcnvrtr setsettings '[{
         *     "max_fee": 30000,
         *     "multi_token": "smarttokens1",
         *     "network": "thisisbancor",
         *     "staking": ""
         * }]' -p bancorcnvrtr
         */
        [[eosio::action]]
        void setsettings( const BancorConverter::settings_t params );

        /**
         * @brief may only set staking/voting contract for this multi-converter once
         * @param currency - currency converter symbol code
         * @param protocol_feature - protocol feature
         * @param enabled - (true/false) to be enabled
         */
        [[eosio::action]]
        void activate( const symbol_code currency, const name protocol_feature, const bool enabled );

        // the 4 actions below updates the converter settings, can only be called by the converter owner after creation

        /**
         * @brief change converter's owner
         * @param currency - the currency symbol governed by the converter
         * @param new_owner - converter's new owner
         */
        [[eosio::action]]
        void updateowner(symbol_code currency, name new_owner);

        /**
         * @brief updates the converter fee
         * @param currency - the currency symbol governed by the converter
         * @param fee - the new fee % for this converter, must be lower than the maximum fee, 0-1000000
         */
        [[eosio::action]]
        void updatefee(symbol_code currency, uint64_t fee);

        /**
         * @brief initializes a new reserve in the converter
         * @param converter_currency_code - the currency code of the currency governed by the converter
         * @param currency - reserve token currency symbol
         * @param contract - reserve token contract name
         * @param ratio - reserve ratio, percentage, 0-1000000
         */
        [[eosio::action]]
        void setreserve(symbol_code converter_currency_code, symbol currency, name contract, uint64_t ratio);

        /**
         * @brief deletes an empty reserve in the converter
         * @param converter - the currency code of the smart token governed by the converter
         * @param currency - reserve token currency code
         */
        [[eosio::action]]
        void delreserve(symbol_code converter, symbol_code reserve);

        /**
         * @brief called by liquidity providers withdrawing "temporary balances" before `fund`ing them into the reserve
         * @param sender - sender of the quantity
         * @param quantity - amount to decrease the supply by (in the smart token)
         * @param converter_currency_code - the currency code of the currency governed by the converter
         */
        [[eosio::action]]
        void withdraw(name sender, asset quantity, symbol_code converter_currency_code);

        /**
         * @brief buys smart tokens with all connector tokens using the same percentage
         * @details i.e. if the caller increases the supply by 10%, it will cost an amount equal to
         * 10% of each connector token balance
         * can only be called if the total ratio is exactly 100%
         * @param  sender - sender of the quantity
         * @param quantity - amount to increase the supply by (in the smart token)
         */
        [[eosio::action]]
        void fund(name sender, asset quantity);

        /**
         * @brief transfer intercepts with standard transfer args
         * @details `memo` containing a keyword following a semicolon at the end of the conversion path indicates special kind of transfer:
         * - e.g. transferring smart tokens with keyword "liquidate", or
         * - transferring reserve tokens with keyword "fund"
         * @param from - the sender of the transfer
         * @param to - the receiver of the transfer
         * @param quantity - the quantity for the transfer
         * @param memo - the memo for the transfer
         */
        [[eosio::on_notify("*::transfer")]]
        void on_transfer(name from, name to, asset quantity, string memo);

        /**
         * @brief log event
         * @details inline action to record log events
         * @param event - emit event
         * @param version - emit event version
         * @param data - emit event data
         */
        [[eosio::action]]
        void log( const string event, const string version, const map<string, string> data );

        [[eosio::action]]
        void migrate( const set<symbol_code> currencies );

        [[eosio::action]]
        void cleartables( const set<symbol_code> currencies );

        /*! \cond DOCS_EXCLUDE */
        typedef eosio::singleton<"settings"_n, settings_t> settings;
        typedef eosio::multi_index<"converters"_n, converter_t> converters;
        typedef eosio::multi_index<"reserves"_n, reserve_t> reserves;
        typedef eosio::multi_index<"accounts"_n, account_t,
            indexed_by<"bycnvrt"_n, const_mem_fun <account_t, uint128_t, &account_t::by_cnvrt >>
        > accounts;

        // migration table
        typedef eosio::multi_index<"converter.v2"_n, converter_v2_t> converters_v2;

        /*! \endcond */

        // Action wrappers
        using log_action = action_wrapper<"log"_n, &BancorConverter::log>;
        using create_action = action_wrapper<"create"_n, &BancorConverter::create>;
        using close_action = action_wrapper<"delconverter"_n, &BancorConverter::delconverter>;
        using setsettings_action = action_wrapper<"setsettings"_n, &BancorConverter::setsettings>;
        using updateowner_action = action_wrapper<"updateowner"_n, &BancorConverter::updateowner>;
        using updatefee_action = action_wrapper<"updatefee"_n, &BancorConverter::updatefee>;
        using setreserve_action = action_wrapper<"setreserve"_n, &BancorConverter::setreserve>;
        using delreserve_action = action_wrapper<"delreserve"_n, &BancorConverter::delreserve>;
        using withdraw_action = action_wrapper<"withdraw"_n, &BancorConverter::withdraw>;
        using fund_action = action_wrapper<"fund"_n, &BancorConverter::fund>;
    private:
        void convert(name from, asset quantity, string memo, name code);
        std::tuple<asset, double> calculate_return(const extended_asset from_token, const extended_symbol to_token, const string memo, const symbol currency, const uint64_t fee, const name multi_token);
        void apply_conversion(memo_structure memo_object, extended_asset from_token, extended_asset to_return, symbol converter_currency);

        const reserve_t& get_reserve(symbol_code symbl, symbol_code converter_currency);
        bool is_converter_active(symbol_code converter);

        void mod_reserve_balance(symbol converter_currency, asset value, int64_t pending_supply_change = 0);
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

        asset get_supply(name contract, symbol_code sym);

        double calculate_purchase_return(double balance, double deposit_amount, double supply, int64_t ratio);
        double calculate_sale_return(double balance, double sell_amount, double supply, int64_t ratio);
        double quick_convert(double balance, double in, double toBalance);
        double calculate_liquidate_return(double liquidation_amount, double supply, double reserve_balance, double total_ratio);
        double calculate_fund_cost(double funding_amount, double supply, double reserve_balance, double total_ratio);

        static uint128_t _by_cnvrt( asset balance, symbol_code converter_currency_code ) {
           return ( uint128_t{ balance.symbol.code().raw() } << 64 ) | converter_currency_code.raw();
        }
        constexpr static double MAX_RATIO = 1000000.0;
        constexpr static double MAX_FEE = 1000000.0;
        constexpr static double MAX_INITIAL_MAXIMUM_SUPPLY_RATIO = 0.1;

        constexpr static double DEFAULT_MAX_SUPPLY = 10000000000.0000;
        constexpr static uint8_t DEFAULT_TOKEN_PRECISION = 4;

        // utils
        double asset_to_double( const asset quantity );
        asset double_to_asset( const double amount, const symbol sym );

        // log
        void emit_conversion_event(
            const symbol_code converter_currency_symbol,
            const string memo,
            const name from_contract,
            const symbol_code from_symbol,
            const name to_contract,
            const symbol_code to_symbol,
            const double amount,
            const double to_return,
            const double conversion_fee
        );
        void emit_price_data_event(
            const symbol_code converter_currency_symbol,
            const double smart_supply,
            const name reserve_contract,
            const symbol_code reserve_symbol,
            const double reserve_balance,
            const double reserve_ratio
        );
        void emit_conversion_fee_update_event(
            const symbol_code converter_currency_symbol,
            const uint64_t prev_fee,
            const uint64_t new_fee
        );

        // migration
        void erase_converters_v1( const symbol_code symcode );
        void erase_reserves_v1( const symbol_code symcode );
        void erase_converters_v1_scoped( const symbol_code symcode );

        template <typename T>
        void clear_table( T& table );


}; /** @}*/
