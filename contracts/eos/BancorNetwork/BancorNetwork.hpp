
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

/// the account which originated the entire conversion path
/// the final destination of the entire conversion path
/// the contract where this conversion was processed
/// the return from the conversion in BNT before fee deduction
/// the account that was paid the affiliate fee
/// the amount of that was paid as affiliate fee 
#define EMIT_AFFILIATE_EVENT(sender, destination, from_contract, fee_account, to_amount, fee_amount){ \
    START_EVENT("affiliate", "1.0") \
    EVENTKV("sender", sender) \
    EVENTKV("destination", destination) \
    EVENTKV("from_contract", from_contract) \
    EVENTKV("affiliate_account", fee_account) \
    EVENTKV("return", to_amount) \
    EVENTKVL("affiliate_fee", fee_amount) \
    END_EVENT() \
}

/**
 * @defgroup bancornetwork BancorNetwork
 * @ingroup bancorcontracts
 * @brief The BancorNetwork contract is the main entry point for bancor token conversions.
 * @details It also allows converting between any token in the bancor network to any other token
 * in a single transaction by providing a conversion path.
 * A note on conversion path -
 * Conversion path is a data structure that's used when converting a token to another token in the bancor network
 * when the conversion cannot necessarily be done by single converter and might require multiple 'hops'.
 * The path defines which converters should be used and what kind of conversion should be done in each step.
 * The path format doesn't include complex structure and instead, it is represented by a space delimited
 * list of values in which each 'hop' is represented by a 2-tuple - converter & 'to' token symbol.
 * The 'from' token is the token that was sent the the contract with the transfer action.
 * Format:
 * [converter account, to token symbol, converter account, to token symbol...]
 * For example, in order to convert 10 EOS into BNT, the caller needs to transfer 10 EOS to the contract
 * and provide the following memo:
 * 1,bnt2eoscnvrt BNT,1.0000000000,receiver_account_name
 * @{
*/
CONTRACT BancorNetwork : public eosio::contract {
    public:
        using contract::contract;

        /**
         * @brief set the maximum affliate fee for all chained BNT conversions
         * @param max_affiliate_fee - what network owner determines to be the maximum 
         */
        ACTION setmaxfee(uint64_t max_affiliate_fee);

        /**
         * @brief transfer intercepts
         * @details memo is in csv format, values -
         * version          version number, currently 1
         * path             conversion path, see description above
         * minimum return   conversion minimum return amount, the conversion will fail if the amount returned is lower than the given amount
         * target account   account to receive the conversion return
         */
        [[eosio::on_notify("*::transfer")]]
        void on_transfer(name from, name to, asset quantity, string memo);
        using transfer_action = action_wrapper<name("transfer"), &BancorNetwork::on_transfer>;
    
    private:
        TABLE settings_t {
            bool enabled;
            uint64_t max_fee;
            
            uint64_t primary_key() const { return "settings"_n.value; }
        };
        typedef eosio::multi_index<"settings"_n, settings_t> settings;
    
        tuple<asset, memo_structure> pay_affiliate(name from, asset quantity, memo_structure memo);
        
        bool isConverter(name converter);
        void verify_min_return(asset quantity, string min_return);
        void verify_entry(name account, name currency_contract, symbol currency);
};
/** @}*/ // end of @defgroup bancornetwork BancorNetwork
