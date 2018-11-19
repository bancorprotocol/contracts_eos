#include <algorithm>
#include <math.h>
#include "./BancorX.hpp"
#include "../Common/common.hpp"
#include "eosiolib/crypto.h"

using namespace eosio;

ACTION BancorX::init(name x_token_name, uint64_t min_limit, uint64_t limit_inc, uint64_t max_issue_limit, uint64_t max_destroy_limit) {
    require_auth(_self);

    settings settings_table(_self, _self.value);
    bool settings_exists = settings_table.exists();

    eosio_assert(!settings_exists, "settings already defined");
    eosio_assert(min_limit >= 0, "minimum limit must be non-negative");
    eosio_assert(min_limit <= max_issue_limit, "minimum limit must be lower or equal than the maximum issue limit");
    eosio_assert(min_limit <= max_destroy_limit, "minimum limit must be lower or equal than the maximum destroy limit");
    eosio_assert(limit_inc > 0, "limit increment must be positive");
    eosio_assert(max_issue_limit >= 0, "maximum issue limit must be non-negative");
    eosio_assert(max_destroy_limit >= 0, "maximum destroy limit must be non-negative");

    settings_table.set(settings_t{ 
        x_token_name,
        false,
        false,
        min_limit,
        limit_inc,
        max_issue_limit,
        max_issue_limit,
        (current_time() / 500000),
        max_destroy_limit,
        max_destroy_limit,
        (current_time() / 500000)
        }, _self);
}

ACTION BancorX::update(uint64_t min_limit, uint64_t limit_inc, uint64_t max_issue_limit, uint64_t max_destroy_limit) {
    require_auth(_self);

    eosio_assert(min_limit >= 0, "minimum limit must be non-negative");
    eosio_assert(min_limit <= max_issue_limit, "minimum limit must be lower or equal than the maximum issue limit");
    eosio_assert(min_limit <= max_destroy_limit, "minimum limit must be lower or equal than the maximum destroy limit");
    eosio_assert(limit_inc > 0, "limit increment must be positive");
    eosio_assert(max_issue_limit >= 0, "maximum issue limit must be non-negative");
    eosio_assert(max_destroy_limit >= 0, "maximum destroy limit must be non-negative");

    settings settings_table(_self, _self.value);
    auto st = settings_table.get();
    st.max_issue_limit = max_issue_limit;
    st.min_limit = min_limit;
    st.limit_inc = limit_inc;
    st.max_destroy_limit = max_destroy_limit;

    settings_table.set(st, _self);
}

ACTION BancorX::enablerpt(bool enable) {
    require_auth(_self);

    settings settings_table(_self, _self.value);
    auto st = settings_table.get();
    st.rpt_enabled = enable;
    settings_table.set(st, _self);
}

ACTION BancorX::enablext(bool enable) {
    require_auth(_self);

    settings settings_table(_self, _self.value);
    auto st = settings_table.get();
    st.xt_enabled = enable;
    settings_table.set(st, _self);
}

ACTION BancorX::addreporter(name reporter) {
    require_auth(_self);
    reporters reporters_table(_self, _self.value);
    auto it = reporters_table.find(reporter.value);

    eosio_assert(it == reporters_table.end(), "reporter already defined");
    
    reporters_table.emplace(_self, [&](auto& s) {
        s.reporter  = reporter;
    });
}

ACTION BancorX::rmreporter(name reporter) {
    require_auth(_self);
    reporters reporters_table(_self, _self.value);
    auto it = reporters_table.find(reporter.value);

    eosio_assert(it != reporters_table.end(), "reporter does not exist");
    
    reporters_table.erase(it);
}

ACTION BancorX::reporttx(name reporter, string blockchain, capi_checksum256 hash_lock, name target, asset quantity, string memo, string data) {
    // checks that the reporter signed on the tx
    require_auth(reporter);

    eosio_assert(memo.size() <= 256, "memo has more than 256 bytes");

    settings settings_table(_self, _self.value);
    auto st = settings_table.get();

    eosio_assert(st.rpt_enabled, "reporting is disabled");

    uint64_t prev_issue_limit = st.prev_issue_limit;
    uint64_t prev_issue_time = st.prev_issue_time;
    uint64_t limit_inc = st.limit_inc;

    uint64_t timestamp = current_time() / 500000;
    
    uint64_t current_delta = 0;
    if (timestamp > prev_issue_time)
        current_delta = timestamp - prev_issue_time;

    uint64_t current_limit = std::min(prev_issue_limit + limit_inc * current_delta, st.max_issue_limit);

    eosio_assert(quantity.amount >= st.min_limit, "below min limit");

    // checks that the signer is known reporter
    reporters reporters_table(_self, _self.value);
    auto existing = reporters_table.find(reporter.value);

    eosio_assert(existing != reporters_table.end(), "the signer is not a known reporter");

    uint64_t short_hash_lock = get_short_hash(hash_lock);
    // checks if the reporters limits are valid
    transfers transfers_table(_self, _self.value);
    auto transaction = transfers_table.find(short_hash_lock);

    // first reporter 
    eosio_assert(transaction == transfers_table.end(), "transaction already exists");
    eosio_assert(quantity.amount <= current_limit, "above max limit");
    transfers_table.emplace(_self, [&](auto& s) {
        s.short_hash_lock = short_hash_lock;
        s.expiration      = (current_time() / 500000) + (2400); // + 20 minutes (2400 half-seconds == 1200 seconds == 20 minutes)
        s.target          = target;
        s.quantity        = quantity;
        s.blockchain      = blockchain;
        s.memo            = memo;
        s.data            = data;
    });

    st.prev_issue_limit = current_limit - quantity.amount;
    st.prev_issue_time  = timestamp;
    settings_table.set(st, _self);

    EMIT_TX_REPORT_EVENT(reporter, blockchain, short_hash_lock, target, quantity, memo);
}

ACTION BancorX::releasetkns(string hash_lock_source, string memo) {
    capi_checksum256 hash_lock;
    const char* cstr = hash_lock_source.c_str();
    sha256(cstr, hash_lock_source.size(), &hash_lock);

    uint64_t short_hash_lock = get_short_hash(hash_lock);
    transfers transfers_table(_self, _self.value);
    auto transaction = transfers_table.find(short_hash_lock);

    settings settings_table(_self, _self.value);
    auto st = settings_table.get();

    eosio_assert(transaction != transfers_table.end(),"there\'s no corresponding transaction to the provided hash lock");
    eosio_assert(current_time() / 500000 <= transaction->expiration, "transaction has expired");

    // issue tokens
    action(
        permission_level{ _self, "active"_n },
        st.x_token_name, "issue"_n,
        std::make_tuple(transaction->target, transaction->quantity, memo)
    ).send();

    EMIT_ISSUE_EVENT(transaction->target, transaction->quantity);

    transfers_table.erase(transaction);
}


void BancorX::transfer(name from, name to, asset quantity, string memo) {
    if (from == _self || to != _self)
        return;

    settings settings_table(_self, _self.value);
    auto st = settings_table.get();
    if (_code != st.x_token_name)
        return;

    auto memo_object = parse_memo(memo);
    xtransfer(memo_object.blockchain, from, memo_object.target, quantity, memo_object.hash_lock);
}

void BancorX::xtransfer(string blockchain, name from, string target, asset quantity, capi_checksum256 hash_lock) {
    settings settings_table(_self, _self.value);
    auto st = settings_table.get();

    eosio_assert(st.xt_enabled, "x transfers are disabled");

    uint64_t prev_destroy_limit = st.prev_destroy_limit;
    uint64_t prev_destroy_time = st.prev_destroy_time;
    uint64_t limit_inc = st.limit_inc;

    uint64_t timestamp = current_time() / 500000;

    uint64_t current_delta = 0;
    if (timestamp > prev_destroy_time)
        current_delta = timestamp - prev_destroy_time;

    uint64_t current_limit = std::min(prev_destroy_limit + limit_inc * current_delta, st.max_destroy_limit);

    eosio_assert(quantity.amount >= st.min_limit, "below min limit");
    eosio_assert(quantity.amount <= current_limit, "above max limit");
    
    uint64_t short_hash_lock = get_short_hash(hash_lock);

    deposits deposits_table(_self, _self.value);
    auto dp = deposits_table.find(short_hash_lock);

    eosio_assert(dp == deposits_table.end(), "deposit already defined");
    
    deposits_table.emplace(_self, [&](auto& s) {
        s.short_hash_lock   = short_hash_lock;
        s.sender            = from;
        s.blockchain        = blockchain;
        s.target            = target;
        s.quantity          = quantity;
        s.claimed           = false;
        s.expiration        = current_time() / 500000 + 9000; // + 75 minutes
    });

    action(
        permission_level{ _self, "active"_n },
        st.x_token_name, "retire"_n,
        std::make_tuple(quantity,std::string("destroy on x transfer"))
    ).send();

    st.prev_destroy_limit = current_limit - quantity.amount;
    st.prev_destroy_time  = timestamp;
    settings_table.set(st, _self);

    EMIT_DESTROY_EVENT(from, quantity);
    EMIT_X_TRANSFER_EVENT(blockchain, target, quantity);
}

// TODO: instead of using a 'claim' field, just delete the deposit when it's claimed
ACTION BancorX::claimx(string hash_lock_source) {
    capi_checksum256 hash_lock;
    const char* cstr = hash_lock_source.c_str();
    sha256(cstr, hash_lock_source.size(), &hash_lock);

    uint64_t short_hash_lock = get_short_hash(hash_lock);

    deposits deposits_table(_self, _self.value);
    auto deposit = deposits_table.find(short_hash_lock);

    eosio_assert(deposit != deposits_table.end(),"there\'s no corresponding deposit to the provided hash lock");
    eosio_assert(deposit->claimed == false, "xtransfer deposit was already claimed");

    deposits_table.modify(deposit, _self, [&](auto& d) {
        d.claimed = true;
    });

    settings settings_table(_self, _self.value);
    auto st = settings_table.get();

    if (current_time() / 500000 > deposit->expiration) {
        action(
            permission_level{ _self, "active"_n },
            st.x_token_name, "issue"_n,
            std::make_tuple(deposit->sender, deposit->quantity,std::string("issue on x transfer"))
        ).send();
        
    }

}

    
extern "C" {
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        if (action == "transfer"_n.value && code != receiver) {
            eosio::execute_action(eosio::name(receiver), eosio::name(code), &BancorX::transfer);
        }
    
        if (code == receiver) {
            switch (action) { 
                EOSIO_DISPATCH_HELPER(BancorX, (init)(update)(enablerpt)(enablext)(addreporter)(rmreporter)(reporttx)(claimx)(releasetkns)) 
            }    
        }

        eosio_exit(0);
    }
}
