
#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/system.hpp>
#include <eosio/singleton.hpp>
//#include <boost/container/flat_set.hpp>
#include "../Common/common.hpp"

using namespace eosio;
using namespace std;

#define EMIT_STAKE_EVENT(sender, amount, transfer, receiver) \
  START_EVENT("delegating", "1.0") \
  EVENTKV("sender", sender) \
  EVENTKV("amount", amount) \
  EVENTKV("transfer", transfer) \
  EVENTKVL("receiver", receiver) \
  END_EVENT() \

#define EMIT_UNSTAKE_EVENT(sender, amount, unstake, receiver) \
  START_EVENT("undelegating", "1.0") \
  EVENTKV("sender", sender) \
  EVENTKV("amount", amount) \
  EVENTKV("unstake", unstake) \
  EVENTKVL("receiver", receiver) \
  END_EVENT() \

#define EMIT_VOTE_EVENT(from, value, property, symbol) \
  START_EVENT("voting", "1.0") \
  EVENTKV("from", from) \
  EVENTKV("value", value) \
  EVENTKV("property", property) \
  EVENTKVL("symbol", symbol) \
  END_EVENT() \

CONTRACT MultiStaking : public eosio::contract {  
  public:
    using contract::contract;

    TABLE config_t {
      int32_t  refund_delay_sec = 1; //86400; //secs per day
      int32_t  start_vote_after = 1; //604800; //secs per week
      name     converter;
      bool     enabled;

      EOSLIB_SERIALIZE( config_t, (refund_delay_sec)(start_vote_after)(converter)(enabled) )      
    };
    
    TABLE vote_t { // Scope -> name.value of owner, _self holds median votes
      symbol_code symbl; // vote's associated relay token symbol 
      name        property; // property's name (fee, decay, etc)
      /*
       * For regular accounts this will always be positive
       * For _self it can be negative, until it's time for
       * voting to begin taking effect, then flip (-) sign
       * and begin updating timestamp along with rebalance 
      */
      int32_t     last_vote_ts;
      int64_t     value = -1;
      uint64_t    id;

      uint64_t    primary_key() const { return id; }
      uint128_t   by_symbol()   const { return _by_symbl( symbl, property ); }
    };

    TABLE stake_t { // Scope -> owner of stake
      asset    staked; // change the delegated_ into ints
      uint64_t delegated_in;
      uint64_t delegated_to;
      uint64_t delegated_out;

      uint64_t primary_key() const { return staked.symbol.code().raw(); }
    }; 
    
    /*
     * This table aggregates the weights and opinions
     * of all individual users in order to facilitate
     * efficient, in-place rebalancing of the global
     * weighted medians for various properties
    */
    TABLE weight_t { // Scope -> name.value of property being voted
      symbol_code symbl; // converter currency symbol code
      uint64_t    total; // staked by property's voters
      uint64_t    k = 0; // approximate index of median 
      double      sum_w_k = 0.0; // sum(W[0..k])
      /*
       * Rather than using a flat_map for better locality
       * of reference, collapsing the two variables into one,
       * store weights, values as separate vectors for fastest
       * random access
      */
      // TODO possible future optimization?
      // boost::container::flat_set<uint64_t> Y;
      vector<uint64_t> Y; // all the different votes
      vector<uint64_t> W; // their associated weights
      
      uint64_t primary_key() const { return symbl.raw(); }
    };

    TABLE refund_t { // Scope -> smart token symbol (to ease reading of open requests)
      name     owner;
      asset    stake; 
      int32_t  request_time;
      
      bool     is_empty()    const { return !stake.amount; }
      uint64_t primary_key() const { return owner.value; }
    };

    TABLE delegate_t { // Scope -> delegator
      name      delegate;
      asset     stake;
      uint64_t  id;
  
      uint64_t  primary_key() const { return id; } 
      bool      is_empty()    const { return !stake.amount; }
      uint128_t by_symbol()   const { return _by_symbl( stake.symbol.code(), delegate ); }
    };
  
    typedef eosio::multi_index<"votes"_n, vote_t, 
                         indexed_by<"bysymbl"_n, 
                           const_mem_fun<vote_t, uint128_t, 
                             &vote_t::by_symbol>>> votes;

    typedef eosio::singleton <"config"_n, config_t> configs;
    typedef eosio::multi_index <"config"_n, config_t> dummy;
    typedef eosio::multi_index <"stakes"_n, stake_t> stakes;
    typedef eosio::multi_index <"weights"_n, weight_t> weights;
    typedef eosio::multi_index <"refunds"_n, refund_t> refunds;

    typedef eosio::multi_index <"delegates"_n, delegate_t, 
                         indexed_by<"bysymbl"_n, 
                           const_mem_fun <delegate_t, uint128_t, 
                             &delegate_t::by_symbol >>> delegated;

    // only callable by owner of given multi-converter (with associated multi-token), and this multi-staking contract
    ACTION setconfig(uint32_t refund_delay_sec, // deferral period for refund request exeuction
                     uint32_t start_vote_after, // duration until weighted median voting updates start getting pushed
                     const name& converter, // name of multi-converter contract to be tethered to this multi-staking contract
                     bool enabled); // is staking/voting enabled globally for every converter in given multi-converter
    
    ACTION delegate(const asset& amount, // amount of converter token being delegated / transfered
                    const name& from, // owner of converter tokens, who had previously staked them
                    const name& to, // delegate to whom `from` is delegating / transferring 
                    bool transfer); // flag indicating whether ownership of stake is being transfered

    ACTION undelegate(const name& from, // delegate from whom `to` is undelegating
                      const name& to, // name of delegator, original stake owne who is undelegating
                      bool unstake, // flag indicating whether to transfer out `amount` (unstak via refund request)
                      asset amount); // amount of converter token being undelegated / unstaked
    
    ACTION refund(symbol_code symbl, // symbol of converter given refund is due to
                  const name& owner); // owner of the original stake, refund request

    ACTION vote(const name& property, // name of the property being voted on, e.g. "fee"_n
                const asset& value, // value of the property voting on, symbol of converter
                const name& owner); // name of the voter   

    void deposit(name from, name to, // `to` is token owner, `from` must be the multi-token (contract origating action)
                 asset quantity, //quantity of converter tokens being transferred in
                 string memo); // transfer memo is diregarded here unlike in multi-converter
  
  private:               
    static uint128_t _by_symbl( symbol_code symbl, name index ) { // helper for composite key
      return ( uint128_t{ symbl.raw() } << 64 ) | index.value;
    }
    auto stake(const name& owner, const asset& quantity);
    auto unstake(const name& owner, const asset& quantity);
    auto pullstake(const name& delegate, const name& delegator, const asset& quantity);
    auto putstake(const name& to, const name& from, bool transfer, const asset& quantity);
    // helper for modifying balances related to un/delegatation, and stake transfer
    void modstakes(const name& from, const name& to, const asset& delta, bool transfer);
    // helper for running weighted median algorithm in-place for an owner's position on property
    void rebalance(const name& owner, const symbol_code& symbl,
                   const name& property, int64_t old_median,
                   uint64_t new_stake, int64_t new_vote,
                   uint64_t old_stake, int64_t old_vote);
    // helper for updating a property's total weight
    void retotal(symbol_code symbl, const name& property, int64_t delta);
    // helper for calling rebalance for every property voted by owner
    void refresh(symbol_code symbl, const name& owner, int64_t old_stake, int64_t new_stake);
    // helper for pushing through newly calculated weighted median updates 
    void update(const symbol_code& symbl, const name& property, int64_t new_median);
    // helper for vote validation
    void validate(const asset& quantity, const name& property);
};
