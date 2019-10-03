#pragma once

#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include "../Common/common.hpp"

using namespace eosio;
using std::string;

// events
// triggered when an account reroutes an xtransfer transaction
#define EMIT_TX_REROUTE_EVENT(tx_id, blockchain, target) \
    START_EVENT("txreroute", "1.1") \
    EVENTKV("tx_id",tx_id) \
    EVENTKV("blockchain",blockchain) \
    EVENTKVL("target",target) \
    END_EVENT()

/*
    the XTransferRerouter contract allows rerouting transactions that were
    sent to the BancorX contract with invalid parameters
*/
CONTRACT XTransferRerouter : public contract {
    public:
        using contract::contract;

        TABLE settings_t {
            bool     rrt_enabled;
            EOSLIB_SERIALIZE(settings_t, (rrt_enabled));
        };

        typedef eosio::singleton<"settings"_n, settings_t> settings;
        typedef eosio::multi_index<"settings"_n, settings_t> dummy_for_abi; // hack until abi generator generates correct name
        
        // true to enable rerouting xtransfers, false to disable it
        // note: can only be called by the contract account
        ACTION enablerrt(bool enable);
        
        // allows an account to change xtransfer transaction details if the original transaction
        // parameters were invalid (e.g non-existent destination blockchain/target)
        // note: only the original sender may reroute an invalid transaction
        ACTION reroutetx(uint64_t tx_id,       // unique transaction id
                         string blockchain,    // target blockchain
                         string target);       // target account/address
};
