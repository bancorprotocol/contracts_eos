
#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/system.h>
#include <eosiolib/transaction.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/symbol.hpp>
#include <eosiolib/singleton.hpp>
#include "../Common/common.hpp"
#include "eosiolib/types.h"

using std::string;
using std::vector;

using namespace eosio;

// events
// triggered when an account initiates a cross chain transafer
#define EMIT_X_TRANSFER_EVENT(blockchain, target, quantity) \
    START_EVENT("xtransfer", "1.1") \
    EVENTKV("blockchain",blockchain) \
    EVENTKV("target",target) \
    EVENTKVL("quantity",quantity) \
    END_EVENT()

// triggered when account tokens are destroyed after cross chain transfer initiation
#define EMIT_DESTROY_EVENT(from, quantity) \
    START_EVENT("destroy", "1.1") \
    EVENTKV("from",from) \
    EVENTKVL("quantity",quantity) \
    END_EVENT()

// triggered when a reporter reports a cross chain transfer from another blockchain
#define EMIT_TX_REPORT_EVENT(reporter, blockchain, transaction, target, quantity, memo) \
    START_EVENT("txreport", "1.1") \
    EVENTKV("reporter",reporter) \
    EVENTKV("from_blockchain",blockchain) \
    EVENTKV("transaction",transaction) \
    EVENTKV("target",target) \
    EVENTKV("quantity",quantity) \
    EVENTKVL("memo",memo) \
    END_EVENT()

// triggered when enough reports arrived and tokens are issued to an account and the cross chain transfer is fulfilled
#define EMIT_ISSUE_EVENT(target, quantity) \
    START_EVENT("issue", "1.1") \
    EVENTKV("target",target) \
    EVENTKVL("quantity",quantity) \
    END_EVENT()

/*
    The BancorX contract allows cross chain token transfers.

    There are two processes that take place in the contract -
    - Initiate a cross chain transfer to a target blockchain (destroys tokens from the caller account on EOS)
    - Report a cross chain transfer initiated on a source blockchain (issues tokens to an account on EOS)

    Reporting cross chain transfers works similar to standard multisig contracts, meaning that multiple
    callers are required to report a transfer before tokens are issued to the target account.
*/
CONTRACT BancorX : public contract {
    using contract::contract;
    public:


        TABLE deposit_t {
            uint64_t            short_hash_lock;
            name sender;
            string blockchain;
            string target;
            asset quantity;
            uint64_t expiration;
            bool claimed;
            uint64_t     primary_key() const { return short_hash_lock; }
        };

        TABLE settings_t {
            name     x_token_name;
            bool     rpt_enabled;
            bool     xt_enabled;
            uint64_t min_limit;
            uint64_t limit_inc;
            uint64_t max_issue_limit;
            uint64_t prev_issue_limit;
            uint64_t prev_issue_time;
            uint64_t max_destroy_limit;
            uint64_t prev_destroy_limit;
            uint64_t prev_destroy_time;
            EOSLIB_SERIALIZE(settings_t, (x_token_name)(rpt_enabled)(xt_enabled)(min_limit)(limit_inc)(max_issue_limit)(prev_issue_limit)(prev_issue_time)(max_destroy_limit)(prev_destroy_limit)(prev_destroy_time))
        };

        TABLE transfer_t {
            uint64_t            short_hash_lock;
            uint64_t            expiration;
            name                target;
            asset               quantity;
            string              blockchain;
            string              memo;
            string              data;
            uint64_t     primary_key() const { return short_hash_lock; }
        };

        TABLE reporter_t {
            name reporter;
            uint64_t primary_key() const { return reporter.value; }
        };

        typedef eosio::singleton<"settings"_n, settings_t> settings;
        typedef eosio::multi_index<"settings"_n, settings_t> dummy_for_abi; // hack until abi generator generates correct name
        typedef eosio::multi_index<"transfers"_n, transfer_t> transfers;
        typedef eosio::multi_index<"reporters"_n, reporter_t> reporters;

        typedef eosio::multi_index<"deposits"_n, deposit_t> deposits;

        // initializes the contract settings
        // can only be called once, by the contract account
        ACTION init(name x_token_name,              // cross chain token account
                    uint64_t min_limit,             // minimum amount that can be transferred out (destroyed)
                    uint64_t limit_inc,             // amount the current limit is increased by in each interval
                    uint64_t max_issue_limit,       // maximum amount that can be issued on EOS in a given timespan
                    uint64_t max_destroy_limit);    // maximum amount that can be transferred out (destroyed) in a given timespan

        // updates the contract settings
        // can only be called by the contract account
        ACTION update(uint64_t min_limit,           // new minimum limit
                      uint64_t limit_inc,           // new limit increment
                      uint64_t max_issue_limit,     // new maximum incoming amount
                      uint64_t max_destroy_limit);  // new maximum outgoing amount

        ACTION enablerpt(bool enable);  // true to enable reporting (and thus issuance), false to disable it, can only be called by the contract account
        ACTION enablext(bool enable);   // true to enable cross chain transfers, false to disable them, can only be called by the contract account

        ACTION addreporter(name reporter);  // adds a new reporter, can only be called by the contract account
        ACTION rmreporter(name reporter);   // remnoves an existing reporter, can only be called by the contract account

        // reports an incoming transaction from a different blockchain
        // can only be called by an existing reporter
        ACTION reporttx(name reporter,      // reporter account
                        string blockchain,  // name of the source blockchain
                        capi_checksum256 hash_lock, // hash lock
                        name target,        // target account on EOS
                        asset quantity,     // amount to issue to the target account if the minimum required number of reports is met
                        string memo,        // memo to pass in in the transfer action
                        string data);       // custom source blockchain value, usually a string representing the tx hash on the source blockchain

        ACTION claimx(string hash_lock_source);

        ACTION releasetkns(string hash_lock_source, string memo);

        // transfer intercepts with standard transfer args
        // if the token received is the cross transfers token, initiates a cross transfer
        void transfer(name from, name to, asset quantity, string memo);

    private:
        struct memo_x_transfer {
            std::string version;
            std::string blockchain;
            std::string target;
            capi_checksum256 hash_lock;
        };

        void xtransfer(string blockchain, name from, string target, asset quantity, capi_checksum256 hash_lock);

        uint8_t from_hex(char c) {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            eosio_assert(false, "Invalid hex character");
            return 0;
        }

        size_t from_hex(const std::string& hex_str, char* out_data, size_t out_data_len) {
            auto i = hex_str.begin();
            uint8_t* out_pos = (uint8_t*)out_data;
            uint8_t* out_end = out_pos + out_data_len;
            while (i != hex_str.end() && out_end != out_pos) {
                *out_pos = from_hex((char)(*i)) << 4;
                ++i;
                if (i != hex_str.end()) {
                    *out_pos |= from_hex((char)(*i));
                    ++i;
                }
                ++out_pos;
            }
            return out_pos - (uint8_t*)out_data;
        }

        capi_checksum256 hex_to_checksum256(const std::string& hex_str) {
            eosio_assert(hex_str.length() == 64, "invalid sha256");
            capi_checksum256 checksum;
            from_hex(hex_str, (char*)checksum.hash, sizeof(checksum.hash));
            return checksum;
        }

        uint64_t get_short_hash(capi_checksum256 hash) { // use reference instead
            uint64_t id = 0;

            for(int i=0; i<8; i++) {
                id <<=8;
                id |= hash.hash[i];
            }

            return id;
        }

        memo_x_transfer parse_memo(string memo) {
            auto res = memo_x_transfer();
            auto parts = split(memo, ",");
            res.version = parts[0];
            res.blockchain = parts[1];
            res.target = parts[2];
            res.hash_lock = hex_to_checksum256(parts[3]);
            return res;
        }
};
