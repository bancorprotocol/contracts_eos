
/**
 *  @file
 *  @copyright defined in ../../../LICENSE
 */
#pragma once

#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include "../Common/common.hpp"

using namespace eosio;
using std::string;

/// events triggered when an account reroutes an xtransfer transaction
#define EMIT_TX_REROUTE_EVENT(tx_id, blockchain, target) \
    START_EVENT("txreroute", "1.1") \
    EVENTKV("tx_id",tx_id) \
    EVENTKV("blockchain",blockchain) \
    EVENTKVL("target",target) \
    END_EVENT()

/**
 * @defgroup bancorxtransfer XTransferRerouter
 * @ingroup bancorcontracts
 * @brief XTransferRerouter contract
 * @details allows rerouting transactions sent to BancorX with invalid parameters 
 * @{
*/
CONTRACT XTransferRerouter : public contract {
    public:
        using contract::contract;

        TABLE settings_t {
            bool     rrt_enabled;
        };

        typedef eosio::singleton<"settings"_n, settings_t> settings;
        typedef eosio::multi_index<"settings"_n, settings_t> dummy_for_abi; // hack until abi generator generates correct name
        
        /**
         * @brief can only be called by the contract account 
         * @param enable - true to enable rerouting xtransfers, false to disable it
         */
        ACTION enablerrt(bool enable);
    
        /**
         * @brief only the original sender may reroute an invalid transaction
         * @details allows an account to change xtransfer transaction details if the original transaction
         * parameters were invalid (e.g non-existent destination blockchain/target)
         * @param tx_id - unique transaction id
         * @param blockchain - target blockchain
         * @param target - target account/address
         */
        ACTION reroutetx(uint64_t tx_id, string blockchain, string target);
};
/** @}*/ // end of @defgroup bancorxtransfer XTransferRerouter
