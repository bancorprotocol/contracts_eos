#include <algorithm>
#include <math.h>
#include "./BancorX.hpp"
#include "../Common/common.hpp"

using namespace eosio;

ACTION BancorX::init(name x_token_name, uint64_t min_reporters, uint64_t min_limit, uint64_t limit_inc, uint64_t max_issue_limit, uint64_t max_destroy_limit) {
    require_auth(_self);

    settings settings_table(_self, _self.value);
    bool settings_exists = settings_table.exists();

    eosio_assert(!settings_exists, "settings already defined");
    eosio_assert(min_reporters > 0, "minimum reporters must be positive");
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
        min_reporters,
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

ACTION BancorX::update(uint64_t min_reporters, uint64_t min_limit, uint64_t limit_inc, uint64_t max_issue_limit, uint64_t max_destroy_limit) {
    require_auth(_self);

    eosio_assert(min_reporters > 0, "minimum reporters must be positive");
    eosio_assert(min_limit >= 0, "minimum limit must be non-negative");
    eosio_assert(min_limit <= max_issue_limit, "minimum limit must be lower or equal than the maximum issue limit");
    eosio_assert(min_limit <= max_destroy_limit, "minimum limit must be lower or equal than the maximum destroy limit");
    eosio_assert(limit_inc > 0, "limit increment must be positive");
    eosio_assert(max_issue_limit >= 0, "maximum issue limit must be non-negative");
    eosio_assert(max_destroy_limit >= 0, "maximum destroy limit must be non-negative");

    settings settings_table(_self, _self.value);
    auto st = settings_table.get();
    st.min_reporters = min_reporters;
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

ACTION BancorX::reporttx(name reporter, string blockchain, uint64_t tx_id, uint64_t x_transfer_id, name target, asset quantity, string memo, string data) {
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

    // checks if the reporters limits are valid
    transfers transfers_table(_self, _self.value);
    auto transaction = transfers_table.find(tx_id);

    // first reporter 
    if (transaction == transfers_table.end()) {
        eosio_assert(quantity.amount <= current_limit, "above max limit");
        transfers_table.emplace(_self, [&](auto& s) {
            s.tx_id           = tx_id;
            s.x_transfer_id   = x_transfer_id;
            s.target          = target;
            s.quantity        = quantity;
            s.blockchain      = blockchain;
            s.memo            = memo;
            s.data            = data;
            s.reporters.push_back(reporter);
        });

        st.prev_issue_limit = current_limit - quantity.amount;
        st.prev_issue_time  = timestamp;
        settings_table.set(st, _self);

        EMIT_TX_REPORT_EVENT(reporter, blockchain, tx_id, target, quantity, x_transfer_id, memo);
    }
    else {
        // checks that the reporter didn't already report the transfer
        eosio_assert(std::find(transaction->reporters.begin(), 
                               transaction->reporters.end(),
                               reporter) == transaction->reporters.end(),
                               "the reporter already reported the transfer");

        eosio_assert(transaction->x_transfer_id == x_transfer_id &&
                     transaction->target == target &&
                     transaction->quantity == quantity &&
                     transaction->blockchain == blockchain &&
                     transaction->memo == memo &&
                     transaction->data == data,
                     "transfer data doesn't match");

        transfers_table.modify(transaction, _self, [&](auto& s) {
            s.reporters.push_back(reporter);
        });
        
        EMIT_TX_REPORT_EVENT(reporter, blockchain, tx_id, target, quantity, x_transfer_id, memo);

        // checks if we have minimal reporters for issue
        if (transaction->reporters.size() >= st.min_reporters) {
            // issue tokens
            action(
                permission_level{ _self, "active"_n },
                st.x_token_name, "issue"_n,
                std::make_tuple(transaction->target, transaction->quantity, memo)
            ).send();

            EMIT_X_TRANSFER_COMPLETE_EVENT(target, x_transfer_id);
            EMIT_ISSUE_EVENT(target, quantity);

            transfers_table.erase(transaction);

            if (x_transfer_id) {
                amounts amounts_table(_self, _self.value);
                auto amount = amounts_table.find(x_transfer_id);
                eosio_assert(amount == amounts_table.end(), "x_transfer_id already exists");
                amounts_table.emplace(_self, [&](auto& a)  {
                    a.x_transfer_id = x_transfer_id;
                    a.quantity = quantity;
                });
            }
        }
    }
}

void BancorX::transfer(name from, name to, asset quantity, string memo) {
    if (from == _self || to != _self)
        return;

    settings settings_table(_self, _self.value);
    auto st = settings_table.get();
    if (_code != st.x_token_name)
        return;

    auto memo_object = parse_memo(memo);
    xtransfer(memo_object.blockchain, from, memo_object.target, quantity, memo_object.x_transfer_id);
}

void BancorX::xtransfer(string blockchain, name from, string target, asset quantity, uint64_t x_transfer_id) {
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

    action(
        permission_level{ _self, "active"_n },
        st.x_token_name, "retire"_n,
        std::make_tuple(quantity,std::string("destroy on x transfer"))
    ).send();

    st.prev_destroy_limit = current_limit - quantity.amount;
    st.prev_destroy_time  = timestamp;
    settings_table.set(st, _self);

    EMIT_DESTROY_EVENT(from, quantity);
    EMIT_X_TRANSFER_EVENT(blockchain, target, quantity, x_transfer_id);
}

extern "C" {
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        if (action == "transfer"_n.value && code != receiver) {
            eosio::execute_action(eosio::name(receiver), eosio::name(code), &BancorX::transfer);
        }
    
        if (code == receiver) {
            switch (action) { 
                EOSIO_DISPATCH_HELPER(BancorX, (init)(update)(enablerpt)(enablext)(addreporter)(rmreporter)(reporttx)) 
            }    
        }

        eosio_exit(0);
    }
}
