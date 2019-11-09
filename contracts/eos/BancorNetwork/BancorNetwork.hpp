
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
 * @{
*/

/*! \cond DOCS_EXCLUDE */
CONTRACT BancorNetwork : public eosio::contract { /*! \endcond */
    public:
        using contract::contract;

        ACTION init();

        /**
         * @brief transfer intercepts
         * @details conversion will fail if the amount returned is lower "minreturn" element in the `memo`
         */
        [[eosio::on_notify("*::transfer")]]
        void on_transfer(name from, name to, asset quantity, string memo);
        
    
    private:
        using transfer_action = action_wrapper<name("transfer"), &BancorNetwork::on_transfer>;
        
        TABLE settings_t {
            bool enabled;
            uint64_t max_fee;
            
            uint64_t primary_key() const { return "settings"_n.value; }
        };

        typedef eosio::multi_index<"settings"_n, settings_t> settings;
        bool isConverter(name converter);
};
/** @}*/ // end of @defgroup bancornetwork BancorNetwork
