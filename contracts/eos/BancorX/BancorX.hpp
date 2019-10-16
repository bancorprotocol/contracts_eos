
/**
 *  @file
 *  @copyright defined in ../../../LICENSE
 */
#pragma once

#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/transaction.hpp>
#include <eosio/asset.hpp>
#include <eosio/symbol.hpp>
#include <eosio/singleton.hpp>
#include "../Common/common.hpp"

using namespace eosio;
using namespace std;

/// triggered when an account initiates a cross chain transafer
#define EMIT_X_TRANSFER_EVENT(blockchain, target, quantity, id) \
    START_EVENT("xtransfer", "1.2") \
    EVENTKV("blockchain",blockchain) \
    EVENTKV("target",target) \
    EVENTKV("quantity",quantity) \
    EVENTKVL("id",id) \
    END_EVENT()

/// triggered when account tokens are destroyed after cross chain transfer initiation
#define EMIT_DESTROY_EVENT(from, quantity) \
    START_EVENT("destroy", "1.1") \
    EVENTKV("from",from) \
    EVENTKVL("quantity",quantity) \
    END_EVENT()

/// triggered when a reporter reports a cross chain transfer from another blockchain
#define EMIT_TX_REPORT_EVENT(reporter, blockchain, transaction, target, quantity, x_transfer_id, memo) \
    START_EVENT("txreport", "1.2") \
    EVENTKV("reporter",reporter) \
    EVENTKV("from_blockchain",blockchain) \
    EVENTKV("transaction",transaction) \
    EVENTKV("target",target) \
    EVENTKV("quantity",quantity) \
    EVENTKV("x_transfer_id",x_transfer_id) \
    EVENTKVL("memo",memo) \
    END_EVENT()

/// triggered when final report is succesfully submitted
#define EMIT_X_TRANSFER_COMPLETE_EVENT(target, id) \
    START_EVENT("xtransfercomplete", "1.2") \
    EVENTKV("target", target) \
    EVENTKVL("id", id) \
    END_EVENT()

/// triggered when enough reports arrived and tokens are issued to an account and the cross chain transfer is fulfilled
#define EMIT_ISSUE_EVENT(target, quantity) \
    START_EVENT("issue", "1.1") \
    EVENTKV("target",target) \
    EVENTKVL("quantity",quantity) \
    END_EVENT()

/**
 * @defgroup bancorx BancorX
 * @ingroup bancorcontracts
 * @brief The BancorX contract allows cross chain token transfers.
 * @details There are two processes that take place in the contract:
 * - Initiate a cross chain transfer to a target blockchain (destroys tokens from the caller account on EOS)
 * - Report a cross chain transfer initiated on a source blockchain (issues tokens to an account on EOS)
 * Reporting cross chain transfers works similar to standard multisig contracts, meaning that multiple
 * callers are required to report a transfer before tokens are issued to the target account.
 * @{
*/
CONTRACT BancorX : public contract {
    public:
        using contract::contract;

        TABLE settings_t {
            name     x_token_name;
            bool     rpt_enabled;
            bool     xt_enabled;
            uint64_t min_reporters;
            uint64_t min_limit;
            uint64_t limit_inc;
            uint64_t max_issue_limit;
            uint64_t prev_issue_limit;
            uint64_t prev_issue_time;
            uint64_t max_destroy_limit;
            uint64_t prev_destroy_limit;
            uint64_t prev_destroy_time;
        };

        TABLE transfer_t {
            uint64_t        tx_id;
            uint64_t        x_transfer_id;
            name            target;
            asset           quantity;
            string          blockchain;
            string          memo;
            string          data;
            vector<name>    reporters;
            uint64_t     primary_key() const { return tx_id; }
        };

        TABLE amounts_t {
            uint64_t x_transfer_id;
            name target;
            asset quantity;
            uint64_t primary_key() const { return x_transfer_id; }
        };

        TABLE reporter_t {
            name reporter;
            uint64_t primary_key() const { return reporter.value; }
        };

        typedef eosio::singleton<"settings"_n, settings_t> settings;
        typedef eosio::multi_index<"settings"_n, settings_t> dummy_for_abi; // hack until abi generator generates correct name
        typedef eosio::multi_index<"transfers"_n, transfer_t> transfers;
        typedef eosio::multi_index<"amounts"_n, amounts_t> amounts;
        typedef eosio::multi_index<"reporters"_n, reporter_t> reporters;

        /**
         * @brief initializes the contract settings
         * @details can only be called once, by the contract account
         * @param x_token_name - cross chain token account
         * @param min_reporters - minimum required number of reporters to fulfill a cross chain transfer
         * @param min_limit - minimum amount that can be transferred out (destroyed)
         * @param max_issue_limit - maximum amount that can be issued on EOS in a given timespan
         * @param max_destroy_limit - maximum amount that can be transferred out (destroyed) in a given timespan
         */
        ACTION init(name x_token_name, uint64_t min_reporters, uint64_t min_limit, uint64_t limit_inc, uint64_t max_issue_limit, uint64_t max_destroy_limit);

        /**
         * @brief updates the contract settings
         * @details can only be called by the contract account
         * @param min_reporters - new minimum required number of reporters
         * @param min_limit - new minimum limit
         * @param limit_inc - new limit increment
         * @param max_issue_limit - new maximum incoming amount
         * @param max_destroy_limit - new maximum outgoing amount
         */
        ACTION update(uint64_t min_reporters,uint64_t min_limit, uint64_t limit_inc, uint64_t max_issue_limit, uint64_t max_destroy_limit); 

        /**
         * @brief can only be called by the contract account
         * @param enable - true to enable reporting (and thus issuance), false to disable it
         */
        ACTION enablerpt(bool enable);
        
        /**
         * @brief can only be called by the contract account
         * @param enable - true to enable cross chain transfers, false to disable them
         */
        ACTION enablext(bool enable);

        /**
         * @brief adds a new reporter, can only be called by the contract account
         * @param reporter - name of the reporter
         */
        ACTION addreporter(name reporter);
        
        /**
         * @brief remnoves an existing reporter, can only be called by the contract account
         * @param reporter - name of the reporter
         */
        ACTION rmreporter(name reporter);

        /**
         * @brief reports an incoming transaction from a different blockchain
         * @details can only be called by an existing reporter
         * @param reporter - reporter account
         * @param blockchain - name of the source blockchain
         * @param tx_id - unique transaction id on the source blockchain
         * @param x_transfer_id - unique (if non zero) pre-determined id (unlike _txId which is determined after the transactions been mined)
         * @param target - target account on EOS
         * @param quantity - amount to issue to the target account if the minimum required number of reports is met
         * @param memo - memo to pass in in the transfer action
         * @param data - custom source blockchain value, usually a string representing the tx hash on the source blockchain
         */ 
        ACTION reporttx(name reporter, string blockchain, uint64_t tx_id, uint64_t x_transfer_id, name target, asset quantity, string memo, string data);

        /**
         * @brief closes row in amounts table, can only be called by bnt token contract or self
         * @param x_transfer_id - the transfer id
         */
        ACTION clearamount(uint64_t x_transfer_id);

        /**
         * @brief transfer intercepts with standard transfer args
         * @details if the token received is the cross transfers token, initiates a cross transfer
         * @param from - the sender of the transfer
         * @param to - the receiver of the transfer
         * @param quantity - the quantity for the transfer
         * @param memo - the memo for the transfer
         */
        [[eosio::on_notify("*::transfer")]]
        void on_transfer(name from, name to, asset quantity, string memo);
        using transfer_action = action_wrapper<name("transfer"), &BancorX::on_transfer>;

    private:
        struct memo_x_transfer {
            std::string version;
            std::string blockchain;
            std::string target;
            std::string x_transfer_id;
        };

        void xtransfer(string blockchain, name from, string target, asset quantity, std::string x_transfer_id);

        memo_x_transfer parse_memo(string memo) {
            auto res = memo_x_transfer();
            auto parts = split(memo, ",");
            res.version = parts[0];
            res.blockchain = parts[1];
            res.target = parts[2];
            res.x_transfer_id = parts[3];
            return res;
        }
};
/** @}*/ // end of @defgroup bancorx BancorX
