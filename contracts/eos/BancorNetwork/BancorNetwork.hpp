
/**
 *  @file
 *  @copyright defined in ../../../LICENSE
 */
#pragma once

#include <eosio/eosio.hpp>
#include <eosio/transaction.hpp>
#include <eosio/asset.hpp>

using namespace eosio;
using namespace std;

/**
 * @defgroup BancorNetwork BancorNetwork
 * @brief The BancorNetwork contract is the main entry point for Bancor token conversions.
 * @details This contract allows converting between the 'from' token being transfered into the contract and any other token in the Bancor network,
 * initiated by submitting a single transaction and providing into `on_transfer` a "conversion path" in the `memo`:
 * - The path is most useful when the conversion must be routed through multiple "hops".
 * - The path defines which converters should be used and what kind of conversion should be done in each "hop".
 * - The path format doesn't include complex structure; it is represented by a space-delimited
 * list of values in which each 'hop' is represented by a pair -- converter s& 'to' token symbol. Here is the format:
 * > [converter account, to token symbol, converter account, to token symbol...]
 * - For example, in order to convert 10 EOS into BNT, the caller needs to transfer 10 EOS to the contract
 * and provide the following memo:
 * > `1,bnt2eoscnvrt BNT,1.0000000000,receiver_account_name`
 * - Optionally, an affiliate fee and affiliate account may be included (either both or neither) as such:
 * > `1,PATH,1.0000000000,receiver_account_name,affiliate_account_name,affiliate_fee`
 * @{
*/

/**
 * @details
 * - the account which originated the entire conversion path
 * - the final destination of the entire conversion path
 * - the contract where this conversion was processed
 * - the return from the conversion in BNT before fee deduction
 * - the account that was paid the affiliate fee
 * - the amount of that was paid as affiliate fee
 */
#define EMIT_AFFILIATE_FEE_EVENT(trader, from_contract, fee_account, to_amount, fee_amount){ \
    START_EVENT("affiliate", "1.0") \
    EVENTKV("trader", trader) \
    EVENTKV("from_contract", from_contract) \
    EVENTKV("affiliate_account", fee_account) \
    EVENTKV("return", to_amount) \
    EVENTKVL("affiliate_fee", fee_amount) \
    END_EVENT() \
}

/*! \cond DOCS_EXCLUDE */
CONTRACT BancorNetwork : public eosio::contract { /*! \endcond */
    public:
        using contract::contract;

        /**
         * @defgroup Network_Settings_Table Settings Table
         * @brief This table stores the settings for the entire Bancor network
         * @{
         *//*! \cond DOCS_EXCLUDE */
            TABLE settings_t { /*! \endcond */

                /**
                 * @brief maximum affiliate fee possible for multiConverters to collect
                 */
                uint64_t max_fee;

                /**
                 * @brief network token contract name
                 */
                name network_token;

                /*! \cond DOCS_EXCLUDE */
                uint64_t primary_key() const { return "settings"_n.value; }
                /*! \endcond */

            }; /** @}*/

        /**
         * @brief set the maximum affliate fee for all chained BNT conversions
         * @param max_affiliate_fee - what network owner determines to be the maximum
         */
        ACTION setmaxfee(uint64_t max_affiliate_fee);

        /**
         * @brief sets the network token contract
         * @param network_token - network token contract account
         */
        ACTION setnettoken(name network_token);

        /**
         * @brief transfer intercepts
         * @details conversion will fail if the amount returned is lower "minreturn" element in the `memo`
         */
        [[eosio::on_notify("*::transfer")]]
        void on_transfer(name from, name to, asset quantity, string memo);

    private:
        using transfer_action = action_wrapper<name("transfer"), &BancorNetwork::on_transfer>;
        typedef eosio::multi_index<"settings"_n, settings_t> settings;

        tuple<asset, memo_structure> pay_affiliate(name from, asset quantity, uint64_t max_fee, memo_structure memo);

        void verify_min_return(asset quantity, string min_return);
        void verify_entry(name account, name currency_contract, symbol currency);

}; /** @}*/
