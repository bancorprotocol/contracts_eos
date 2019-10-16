/**
 *  @file
 *  @copyright defined in ../../../LICENSE
 */

#include "../MultiConverter/MultiConverter.hpp"
#include "MultiStaking.hpp"

ACTION MultiStaking::setconfig(uint32_t refund_delay_sec, uint32_t start_vote_after, const name& converter, bool enabled) {
  require_auth(get_self());

  config_t new_config;
  configs configs_table(get_self(), get_self().value);
  if (configs_table.exists())
    new_config = configs_table.get();
  else { // once set converter can't be changed
    MultiConverter::settings settings_table(converter, converter.value);
    const auto& st = settings_table.get("settings"_n.value, "non-existant multi-converter");
    check(st.staking == get_self(), "multi-converter not linked");
    new_config.converter = converter;
  }
  new_config.refund_delay_sec = refund_delay_sec;
  new_config.start_vote_after = start_vote_after;
  new_config.enabled = enabled;

  configs_table.set(new_config, get_self());
}

ACTION MultiStaking::delegate(const asset& amount, const name& from, const name& to, bool transfer) { 
  require_auth(from);
  require_recipient(to);
  
  check(amount.is_valid(), "invalid quantity");
  check(amount.amount > 0, "cannot un/delegate this amount");
  check(!transfer || from != to, "only 'stake' may transfer to self"); 
  // If transfer set to true, this transfers stake ownership, 
  // to the receiver, otherwise it's only delegated by "from"
  // Now the receiver has the right to unstake the relay token
  // in addition to gaining the quantity as weight for voting. 
  modstakes(from, to, amount, transfer);
  
  EMIT_STAKE_EVENT(from, amount, transfer, to);
}

ACTION MultiStaking::undelegate(const name& from, const name& to, bool unstake, asset amount) { 
  require_auth(from);
  require_recipient(to); // if stake was delegated
  
  check(amount.is_valid(), "invalid quantity");
  check(amount.amount > 0, "cannot un/delegate this amount");
  // if transfer is true, unstake via refund
  // otherwise just undelegate from receiver
  // if amount was delegated from to receiver,
  // before creating refund request for from
  // decrement receiver's delegated amount 
  modstakes(from, to, -amount, unstake);

  EMIT_UNSTAKE_EVENT(from, amount, unstake, to);
}

// Call directly if deferred transaction calling it doesnt go through
ACTION MultiStaking::refund(symbol_code symbl, const name& owner) {
  refunds refunds_tbl(get_self(), symbl.raw());
  const auto& req = refunds_tbl.get(owner.value, "refund request not found");
  configs configs_table(get_self(), get_self().value);
  const auto& cfg = configs_table.get();  

  int32_t current_time = current_time_point().sec_since_epoch();
  check(req.request_time + cfg.refund_delay_sec <= current_time,
         "refund is not available for retreival yet" 
      );
  MultiConverter::settings settings_table(cfg.converter, cfg.converter.value);
  const auto& st = settings_table.get("settings"_n.value, "non-existant multi_converter");
  action(permission_level{ get_self(), name("active") },
    st.multi_token, "transfer"_n, std::make_tuple(
      get_self(), owner, req.stake, string("unstake") 
   )).send();

  refunds_tbl.erase(req);
}

ACTION MultiStaking::vote(const name& property, const asset& value, const name& owner) {
  require_auth(owner);
  validate(value, property);
  
  votes votes_tbl(get_self(), owner.value);
  votes votes_glbl(get_self(), get_self().value);
  stakes stakes_tbl(get_self(), owner.value);
  
  const auto& its = stakes_tbl.get(value.symbol.code().raw(), "cannot vote if user has no weight");
  uint64_t new_weight = its.delegated_in + its.delegated_to;
  int64_t new_vote = value.amount;

  auto key = _by_symbl(value.symbol.code(), property); // this key is used twice
  auto index = votes_tbl.get_index< "bysymbl"_n >();
  auto itv = index.find(key);

  uint64_t old_weight = 0; // will stay zero if we don't find owner's vote for this property
  int64_t old_vote = -1; // default indicating user's first vote
  if (itv != index.end()) {
    old_vote = itv->value;
    old_weight = new_weight;
    if (new_vote == old_vote) return;
  } else { 
    retotal(value.symbol.code(), property, new_weight);
    votes_tbl.emplace(owner, [&](auto& v) {
      v.value = new_vote;
      v.property = property;
      v.symbl = value.symbol.code();
      v.id = votes_tbl.available_primary_key();
    });
    itv = index.find(key);
  }
  index.modify(itv, same_payer, [&](auto& v) {
    v.last_vote_ts = current_time_point().sec_since_epoch();
    v.value = new_vote;
  });
  auto index_g = votes_glbl.get_index< "bysymbl"_n >();
  auto itg = index_g.find(key); // key from before (second usage here)
  
  rebalance(owner, value.symbol.code(), property, itg->value, 
            new_weight, new_vote, old_weight, old_vote);
  
  EMIT_VOTE_EVENT(owner, new_vote, property, value.symbol.code());
}

void MultiStaking::on_transfer(name from, name to, asset quantity, string memo) {
  if (to != get_self() || from == get_self() || 
      from == "eosio.ram"_n || from == "eosio.stake"_n || from == "eosio.rex"_n) return;

  configs configs_table(get_self(), get_self().value);
  check(to == get_self(), "can only 'stake' to self");
  check(configs_table.exists(), "setconfig first");
  
  const auto& cfg = configs_table.get(); 
  MultiConverter::settings settings_table(cfg.converter, cfg.converter.value);
  const auto& st = settings_table.get("settings"_n.value, "multi-converter doesn't exist");

  check(st.multi_token.value == get_first_receiver().value, "multi-token mismatch");
  // transfer flag is always true
  // unlike in stake action where
  // given that from == receiver
  // transfer flag can't be true
  modstakes(from, from, quantity, true);
}

auto MultiStaking::stake(const name& owner, const asset& quantity) { 
  stakes stakes_tbl(get_self(), owner.value);
  auto its = stakes_tbl.find(quantity.symbol.code().raw());
  if (its == stakes_tbl.end()) { // contract temporarily pays for ram
    its = stakes_tbl.emplace(get_self(), [&](auto& s) { 
      s.delegated_in = quantity.amount;      
      s.staked = quantity;
    });
  } else {
    stakes_tbl.modify(its, same_payer, [&](auto& s) {
      s.delegated_in += quantity.amount;
      s.staked += quantity;
    });
  }
  return its;
}

auto MultiStaking::unstake(const name& owner, const asset& quantity) {
  stakes stakes_tbl(get_self(), owner.value);
  auto its = stakes_tbl.require_find(quantity.symbol.code().raw(), "stake doesn't exist");
  
  asset available = its->staked;
  available.amount -= its->delegated_out;
  // for stake that is delegated out to become available
  // it must first be undelegated by delegator from delegate
  check(available >= quantity, "can't unstake this much from self");  
  stakes_tbl.modify(its, owner, [&](auto& s) {
    s.delegated_in -= quantity.amount;
    s.staked -= quantity;
  });
  refunds refunds_tbl(get_self(), quantity.symbol.code().raw());
  auto req = refunds_tbl.find(owner.value);
  bool need_deferred_trx = true;
  
  if (req != refunds_tbl.end()) { // need to update refund
    refunds_tbl.modify(req, same_payer, [&](auto& r) {
      r.request_time = current_time_point().sec_since_epoch();
      r.stake += quantity;
    });
    check(req->stake.amount >= 0, "can't have negative refund");
    need_deferred_trx = false;
  } 
  else 
    refunds_tbl.emplace(owner, [&](auto& r) {
      r.owner = owner;
      r.stake = quantity;
      r.request_time = current_time_point().sec_since_epoch();
    });
  if (need_deferred_trx) {
    configs configs_table(get_self(), get_self().value);
    const auto& cfg = configs_table.get();
    eosio::transaction out;
    out.delay_sec = cfg.refund_delay_sec;
    out.actions.emplace_back(permission_level{ get_self(),  "active"_n },
                             get_self(), "refund"_n, std::make_tuple(
                             quantity.symbol.code(), owner));  
    out.send(owner.value, owner, true);
  }
  return its;
}

auto MultiStaking::pullstake(const name& delegate, const name& delegator, const asset& quantity) {
  // for stake that is delegated out to become available
  // it must first be undelegated by delegator from delegate
  delegated delegates(get_self(), delegator.value); 
  auto key = _by_symbl(quantity.symbol.code(), delegate);
  auto index = delegates.get_index<"bysymbl"_n>();
  auto itd = index.find(key);
  check(itd != index.end(), "non-existant delegation");
  index.modify(itd, same_payer, [&](auto& d) {
    d.stake -= quantity;
  });
  check(itd->stake.amount >= 0, "insufficient delegated stake");
  if (itd->is_empty())
    index.erase(itd);

  stakes stakes_to(get_self(), delegator.value);
  stakes stakes_from(get_self(), delegate.value);

  auto its = stakes_to.require_find(quantity.symbol.code().raw(), "delegator doesn't exist");
  auto itr = stakes_from.require_find(quantity.symbol.code().raw(), "delegate doesn't exist");

  check(its->delegated_out >= quantity.amount, "cannot undelegate more than delegated out");
  check(itr->delegated_to >= quantity.amount, "cannot undelegate more than delegated to");
  
  stakes_from.modify(itr, same_payer, [&](auto& r) { // delegate
    r.delegated_to -= quantity.amount;  
  });
  stakes_to.modify(its, delegator, [&](auto& s) { // delegator
    s.delegated_out -= quantity.amount;
    s.delegated_in += quantity.amount;
  });
  return std::make_tuple(itr, its);
}

auto MultiStaking::putstake(const name& to, const name& from, bool transfer, const asset& quantity) {
  stakes stakes_from(get_self(), from.value);
  stakes stakes_to(get_self(), to.value);
  name payer = has_auth(to) ? to : from;

  auto its = stakes_from.require_find(quantity.symbol.code().raw(), "delegator doesn't exist");
  auto itr = stakes_to.find(quantity.symbol.code().raw());

  asset available = its->staked;
  available.amount -= its->delegated_out;
  check(available >= quantity, "cannot delegate out more than staked");  
  
  if (itr == stakes_to.end()) // sender can also pay receiver's RAM
    itr = stakes_to.emplace(payer, [&](auto& r) { 
      r.staked.symbol = quantity.symbol;
    });
  stakes_from.modify(its, from, [&](auto& s) {
    if (transfer)
      s.staked -= quantity;
    else
      s.delegated_out += quantity.amount;
    s.delegated_in -= quantity.amount;
  });
  stakes_to.modify(itr, same_payer, [&](auto& r) {
    if (transfer) {
      r.delegated_in += quantity.amount;
      r.staked += quantity;
    } else 
      r.delegated_to += quantity.amount;
  });
  if (!transfer) {
    delegated delegates(get_self(), from.value); 
    auto key = _by_symbl(quantity.symbol.code(), to);
    auto index = delegates.get_index<"bysymbl"_n>();
    auto itd = index.find(key);
    if (itd == index.end())
      delegates.emplace(payer, [&](auto& d) {
        d.id = delegates.available_primary_key();
        d.stake = quantity;
        d.delegate = to;
      });
    else
      index.modify(itd, same_payer, [&](auto& d) {
        d.stake += quantity;
      });
  }
  return std::make_tuple(its, itr);
}

void MultiStaking::modstakes(const name& from, const name& to, const asset& delta, bool transfer) {
  bool to_self = to == from;
  bool is_undelegating = delta.amount < 0;
  bool is_delegating = !is_undelegating;
  
  uint64_t old_weight_from = 0;
  uint64_t old_weight_to = 0;

  stakes stakes_from(get_self(), from.value);
  stakes stakes_to(get_self(), to.value);

  auto its = stakes_from.find(delta.symbol.code().raw());
  auto itr = stakes_to.find(delta.symbol.code().raw());
  
  if (its != stakes_from.end())
    old_weight_from = its->delegated_in + its->delegated_to;
    
  if (itr != stakes_to.end())
    old_weight_to = itr->delegated_in + itr->delegated_to;
  
  if (to_self) {
    if (is_delegating) {
      check(transfer, "must deposit first");
      its = stake(to, delta);
    } else its = unstake(from, -delta);
  } else {
    if (is_delegating) 
      tie(its, itr) = putstake(to, from, transfer, delta);
    else {
      tie(its, itr) = pullstake(from, to, -delta);
      if (transfer) itr = unstake(to, -delta);
    }
    check(itr->staked.amount == itr->delegated_in + itr->delegated_out,
          "staked minus currently delegated cannot be negative");
    
    int64_t new_weight_to = itr->delegated_in + itr->delegated_to;
    if (old_weight_to && !new_weight_to && !itr->staked.amount) {
      new_weight_to = -1; // in refresh this will signal to release all user's votes
      stakes_to.erase(itr); // user had weight before, not anymore so release row
    }
    refresh(delta.symbol.code(), to, old_weight_to, new_weight_to);  
  }
  check(its->staked.amount == its->delegated_in + its->delegated_out,
        "staked minus currently delegated cannot be negative");

  int64_t new_weight_from = its->delegated_in + its->delegated_to;
  if (old_weight_from && !new_weight_from && !its->staked.amount) {
    new_weight_from = -1; 
    stakes_from.erase(its);
  }
  refresh(delta.symbol.code(), from, old_weight_from, new_weight_from);  
}

/*  Weighted Median Algorithm
 *  Find value of k in range(1, len(Weights)) such that 
 *  sum(Weights[0:k]) = sum(Weights[k:len(Weights)+1])
 *  = sum(Weights) / 2
 *  If there is no such value of k, there must be a value of k 
 *  in the same range range(1, len(Weights)) such that 
 *  sum(Weights[0:k]) > sum(Weights) / 2
*/
void MultiStaking::rebalance(const name& owner, const symbol_code& symbl, 
                             const name& property, int64_t old_median,
                             uint64_t new_stake, int64_t new_vote,
                             uint64_t old_stake, int64_t old_vote) {
                
  weights weights_tbl(get_self(), property.value);
  const auto& itw = weights_tbl.get(symbl.raw(), "no weights for this symbol");
  
  int64_t k = itw.k;
  int64_t median = old_median;
  double sum_w_k = itw.sum_w_k;
  double mid_stake = itw.total / 2.0;
  
  std::vector<uint64_t> Y = itw.Y;
  uint64_t sizeY = Y.size();
  std::vector<uint64_t> W = itw.W;
  uint64_t sizeW = W.size();
  
  check(sizeW == sizeY, "data structures broken");
  auto it = std::find(Y.begin(), Y.end(), new_vote);
  bool appended = it == Y.end();
  if (!appended)
    W.at(it - Y.begin()) += new_stake;
  else if (new_stake != 0) { // this may occur when a user is unstaking
    Y.push_back(new_vote);
    sizeY += 1;
    std::sort(Y.begin(), Y.end()); 

    it = std::find(Y.begin(), Y.end(), new_vote);
    W.insert(W.begin() + (it - Y.begin()), new_stake);
  }
  if (old_vote != -1 && old_stake != 0) { // if not the first time user is voting
    it = std::find(Y.begin(), Y.end(), old_vote);
    check(it != Y.end(), "old vote not found");
    
    auto itr = it - Y.begin();
    W.at(itr) -= old_stake; // clear old weight, even if it's at the same fee
    
    if (W.at(itr) == 0) {
      k -= (itr <= k && k > 0) ? 1 : 0;
      W.erase(W.begin() + itr);
      Y.erase(it);
      sizeY--;
    }
  }
  if (itw.total > 0) { // if total_weight is zero we keep the median at its previous value
    if (sizeY == 1 || new_vote <= median)    
      sum_w_k += new_stake;
    if (old_vote <= median)    
      sum_w_k -= old_stake;
    if (median > new_vote) {
      k += appended && sizeY > 1 ? 1 : 0;
      while(k >= 1 && sum_w_k - W.at(k) >= mid_stake)
        sum_w_k -= W.at(k--);
    } else
      while(sum_w_k < mid_stake)
        sum_w_k += W.at(++k);
    
    median = Y.at(k);
    if (sum_w_k == mid_stake) {
      median += Y.at(k + 1);
      median /= 2;
    }
  } else sum_w_k = 0.0;

  weights_tbl.modify(itw, same_payer, [&](auto& w) { // update values for future iterations
    w.sum_w_k = sum_w_k;       
    w.k = k;
    w.Y = Y;
    w.W = W;
  });
  if (old_median != median)
    update(symbl, property, median);
}

void MultiStaking::refresh(symbol_code symbl, const name& owner, int64_t old_weight, int64_t new_weight) {
  votes votes_tbl(get_self(), owner.value);
  votes votes_glbl(get_self(), get_self().value);

  auto index_tbl = votes_tbl.get_index< "bysymbl"_n >();
  auto index_glbl = votes_glbl.get_index< "bysymbl"_n >();
  
  bool erase_votes = new_weight == -1;
  new_weight *= erase_votes ? 0 : 1;

  // to figure out which parameters are relevant out of all available ones
  // iterate over all lower 64 bits while having constant 64 higher bits 
  uint128_t lower_bound_val = (uint128_t) symbl.raw() << 64; // | 0 implicit
  uint128_t upper_bound_val = ((uint128_t) symbl.raw() << 64) | 0xFFFFFFFFFFFFFFFF;
  auto upper_bound = index_glbl.upper_bound(upper_bound_val);
  
  for (auto itr = index_glbl.lower_bound(lower_bound_val); itr != upper_bound; itr++) {
    auto found = index_tbl.find(_by_symbl(symbl, itr->property));
    if (found != index_tbl.end()) {
      int64_t vote = found->value;    
      retotal(symbl, found->property, new_weight - old_weight);
      rebalance(owner, symbl, found->property, itr->value, new_weight, vote, old_weight, vote);
      if (erase_votes)
        index_tbl.erase(found);
    }
  }
}

void MultiStaking::retotal(symbol_code symbl, const name& property, int64_t delta) {
  weights weights_tbl(get_self(), property.value);
  const auto& itw = weights_tbl.get(symbl.raw(), "no weights");
  // logically can never be < 0 so we don't need to waste time on check
  weights_tbl.modify(itw, same_payer, [&](auto& w) { // update total
    w.total += delta;
  }); 
}

void MultiStaking::update(const symbol_code& symbl, const name& property, int64_t new_median) {
  configs configs_table(get_self(), get_self().value);
  const auto& cfg = configs_table.get();
  
  MultiConverter::converters converters_table(cfg.converter, symbl.raw());
  const auto& cnvrt = converters_table.get(symbl.raw(), "non-existant converter");
  
  auto key = _by_symbl(symbl, property);
  votes votes_glbl(get_self(), get_self().value); 
  auto index = votes_glbl.get_index< "bysymbl"_n >();
  // _self owner scope for converter's property contains median
  
  auto it = index.find(key);
  check(it != index.end(), "converter property not supported");
  
  int32_t last_vote = it->last_vote_ts;
  int32_t current_time = current_time_point().sec_since_epoch();
  
  if (last_vote < 0 && last_vote >= -current_time) 
    last_vote = current_time;
  
  index.modify(it, same_payer, [&](auto& v) {
    v.last_vote_ts = last_vote;
    v.value = new_median; 
  });
  if (last_vote < 0 || !cnvrt.stake_enabled || !cfg.enabled) return;

  switch (property.value) {
    case "fee"_n.value: {
      action(
        permission_level{ get_self(), "active"_n },
        cfg.converter, "updatefee"_n, 
        std::make_tuple(symbl, it->value)
     ).send(); 
      break;
    }
    case "decay"_n.value: {
      /*  TODO: vote decay
       *  Every time a vote is cast we must first "undo" the last vote weight, 
       *  before casting the new vote weight.  Vote weight is calculated as:
       *  staked.amount * 2 ^ (weeks_since_user's_last_vote/weeks_per_year)
       *  Allow the user to vote on the same fee, and not change their stake
       *  the weight is simply heavier due to vote being more recent albeit
       *  no different, therefore delete the "redundant" check below
      */
      break;
    }
    case "delay"_n.value: {
      break; // TODO
    }
    case "ratio"_n.value: {
      break; // TODO
    }
  }
}

void MultiStaking::validate(const asset& quantity, const name& property) {
  check(quantity.is_valid(), "invalid quantity");
  configs configs_table(get_self(), get_self().value);
  check( configs_table.exists(), "setconfig first" );
  
  const auto& cfg = configs_table.get(); 
  MultiConverter::settings settings_table(cfg.converter, cfg.converter.value);
  const auto& st = settings_table.get("settings"_n.value, "multi-converter doesn't exist");

  weights weights_tbl(get_self(), property.value);
  auto itw = weights_tbl.find(quantity.symbol.code().raw());
  if (itw == weights_tbl.end()) // first vote for this property ever 
    weights_tbl.emplace(get_self(), [&](auto& w) {
      w.symbl = quantity.symbol.code();
    });
  votes votes_glbl(get_self(), get_self().value);
  auto key = _by_symbl(quantity.symbol.code(), property); 
  auto index = votes_glbl.get_index< "bysymbl"_n >();
  auto itv = index.find(key);
  if (itv == index.end()) { // first vote for this property ever 
    int32_t current_time = current_time_point().sec_since_epoch();
    votes_glbl.emplace(get_self(), [&](auto& v) {
      v.last_vote_ts = -(current_time + cfg.start_vote_after);
      v.id = votes_glbl.available_primary_key();
      v.symbl = quantity.symbol.code();
      v.property = property;
    });
  }
  switch (property.value) {
    case "fee"_n.value: {
      check(quantity.amount >= 0, "cannot vote this amount");
      check(quantity.amount <= st.max_fee, "can't exceed max fee");
      break;
    }
    case "decay"_n.value: {
      break; // TODO
    }
    case "delay"_n.value: {
      break; // TODO
    }
    case "ratio"_n.value: {
      break; // TODO
    }
    default: check(0, "Expected property fee|decay|delay|ratio");
  }
}
