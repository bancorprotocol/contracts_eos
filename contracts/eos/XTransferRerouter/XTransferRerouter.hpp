
/**
 *  @file
 *  @copyright defined in ../../../LICENSE
 */
#pragma once

#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>

using namespace eosio;
using namespace std;

/**
 * @defgroup BancorXtransfer XTransferRerouter
 * @brief XTransferRerouter contract
 * @details Allows rerouting transactions sent to BancorX with invalid parameters.
 * @{
*/

/// events triggered when an account reroutes an xtransfer transaction
#define EMIT_TX_REROUTE_EVENT(tx_id, blockchain, target) \
    START_EVENT("txreroute", "1.1") \
    EVENTKV("tx_id",tx_id) \
    EVENTKV("blockchain",blockchain) \
    EVENTKVL("target",target) \
    END_EVENT()

/*! \cond DOCS_EXCLUDE */
CONTRACT XTransferRerouter : public contract { /*! \endcond */
    public:
        using contract::contract;

        /**
         * @defgroup Xtransfer_Settings_Table Settings Table
         * @brief Basic minimal settings
         * @{
         *//*! \cond DOCS_EXCLUDE */
            TABLE settings_t { /*! \endcond */
                /// toggle boolean to enable / disable rerouting
                bool rrt_enabled;
            }; /** @}*/

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

    private:
        typedef eosio::singleton<"settings"_n, settings_t> settings;
        typedef eosio::multi_index<"settings"_n, settings_t> dummy_for_abi; // hack until abi generator generates correct name

}; /** @}*/
