
/**
 *  @file
 *  @copyright defined in ../../../LICENSE
 */
#pragma once

#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/system.hpp>
#include <eosio/singleton.hpp>
//#include <boost/container/flat_set.hpp>

using namespace eosio;
using namespace std;

/**
 * @defgroup BancorStaking MultiStaking
 * @brief MultiStaking contract
 * @details allows staking smart tokens from a multi-token and 
 * voting on properties of converters in a multi-converter
 * @{
 */

/// triggered after a staking event occurs related to this multi-converter/multi-staking contract
#define EMIT_STAKE_EVENT(sender, amount, transfer, receiver) \
  START_EVENT("delegating", "1.0") \
  EVENTKV("sender", sender) \
  EVENTKV("amount", amount) \
  EVENTKV("transfer", transfer) \
  EVENTKVL("receiver", receiver) \
  END_EVENT() \

/// triggered after an unstaking event occurs related to this multi-converter/multi-staking contract
#define EMIT_UNSTAKE_EVENT(sender, amount, unstake, receiver) \
  START_EVENT("undelegating", "1.0") \
  EVENTKV("sender", sender) \
  EVENTKV("amount", amount) \
  EVENTKV("unstake", unstake) \
  EVENTKVL("receiver", receiver) \
  END_EVENT() \

/// triggered after a voting event occurs related to this multi-converter/multi-staking contract
#define EMIT_VOTE_EVENT(from, value, property, symbol) \
  START_EVENT("voting", "1.0") \
  EVENTKV("from", from) \
  EVENTKV("value", value) \
  EVENTKV("property", property) \
  EVENTKVL("symbol", symbol) \
  END_EVENT() \

/*! \cond DOCS_EXCLUDE */
CONTRACT MultiStaking : public eosio::contract { /*! \endcond */
  public:
    using contract::contract;

    /** 
     * @defgroup Configs_Table Configs Table 
     * @brief This table stores global parameters affecting voting for all properties tracked by this contract
     * @details Both SCOPE and PRIMARY KEY are `_self`, so this table is effectively a singleton
     * @{
     *//*! \cond DOCS_EXCLUDE */
        TABLE config_t { /*! \endcond */
          /**
           * @brief how long (secs) a user must wait after unstaking tokens to receive them back in their balance
           */
          int32_t refund_delay_sec;
          
          /**
           * @brief this period (in secs) of no-action from when the first vote for a property occurs is to give time for other users to commit their stake and vote
           */
          int32_t start_vote_after;
          
          /**
           * @brief name of the account storing the MultiConverter contract that this staking/voting contract will affect
           */
          name converter;
          
          /**
           * @brief as many converters inside the MultiConverter may use this same contract, having a global kill switch for staking/voting is useful
           */
          bool enabled;

        }; /** @}*/
    
    /** 
     * @defgroup Votes_Table Votes Table
     * @brief This table stores every user's votes for specific properties pertaining to converters and their smart tokens
     * @details SCOPE of this table is `name.value` of the vote owner; `_self` scope is a special case that holds median votes for each property
     * @{
     *//*! \cond DOCS_EXCLUDE */
        TABLE vote_t { /*! \endcond */
          /**
           * @brief converter currency (smart token) symbol code this vote pertains to
           */
          symbol_code symbl; 
          
          /**
           * @brief name of property being voted on (e.g. fee, decay, delay, ratio, etc.)
           */
          name property; 
        
          /**
           * @brief timestamp of the vote owner's last vote
           * @details For regular accounts `last_vote_ts` will always be positive \n 
           *  For `_self` it can be negative, until it's time for
           *  voting to begin taking effect, then flip (-) sign
           *  and begin updating timestamp along with rebalance. 
           */
          int32_t last_vote_ts; 
          
          /**
           * @brief The value of the vote
           */
          int64_t value; 

          /**
           * @brief Dummy primary key
           */
          uint64_t vote_id; 

          /**
           * @brief SECONDARY key of this table is used to get votes for a particular smart token converter's property 
           * @details uint128_t { symbl.raw() } << 64 ) | property.value
           */
          uint128_t by_symbol_property()   const { return _by_symbl( symbl, property ); }

          /*! \cond DOCS_EXCLUDE */
          uint64_t    primary_key() const { return vote_id; }
          /*! \endcond */

        }; /** @}*/

    /** 
     * @defgroup Stakes_Table Stakes Table
     * @brief This table stores every user's stakes, scoped by the stake owner's name.value
     * @details SCOPE of this table is the owner / delegator / delegate of the stake quantities
     * @{
     *//*! \cond DOCS_EXCLUDE */
        TABLE stake_t { /*! \endcond */
          /**
           * @brief quantity of smart tokens owned by user which are currently staked 
           * @details PRIMARY KEY of this table is `staked.symbol.code().raw()`
           */
          asset staked;

          /**
           * @brief how much of the user's stake is accounted towards the user's own voting weight
           */
          uint64_t delegated_in;

          /**
           * @brief how much of the user's total voting weight has been delegated to this user by other users
           */
          uint64_t delegated_to;

          /**
           * @brief how much of the user's stake is delegated out to be accounted into other users' voting weights
           */
          uint64_t delegated_out;

          /*! \cond DOCS_EXCLUDE */
          uint64_t primary_key() const { return staked.symbol.code().raw(); }
          /*! \endcond */

        }; /** @}*/
    
    /**
     * @defgroup Weights_Table Weights Table
     * @brief This table aggregates the weights and opinions of all individual users, scoped by name.value of property being voted
     * @details SCOPE of this table is `_self`. \n 
     * To facilitate efficient, in-place rebalancing of the global weighted medians for various properties, 
     * we store weights and values as separate vectors for fastest random access,
     * rather than using a flat_map to collapsing the two sets into one for better locality of reference
     * @{
     *//*! \cond DOCS_EXCLUDE */
        TABLE weight_t { /*! \endcond */
          /**
           * @brief converter currency symbol code these weights pertain to
           * @details PRIMARY KEY of this table is `symbl_code.raw()`
           */
          symbol_code symbl_code; 

          /**
           * @brief staked by property's voters
           */
          uint64_t total; 
          
          /**
           * @brief approximate index of median 
           */
          uint64_t k; 
          
          /**
           * @brief sum(W[0..k])
           */
          double sum_w_k; 
          
          /**
           * @brief all the distinct votes for given property
           */
          vector<uint64_t> Y; 
          
          /**
           * @brief all the weights associated with the votes
           * @details Possible future optimization boost::container::flat_set<uint64_t>
           */
          vector<uint64_t> W; 
         
          /*! \cond DOCS_EXCLUDE */
          uint64_t primary_key() const { return symbl_code.raw(); }
          /*! \endcond */ 
          
        }; /** @}*/

    /**
     * @defgroup Refunds_Table Refunds Table
     * @brief This table stores refund requests that get created when users unstake their funds, 
     * @details SCOPE of this table is the smart token symbol, `stake.symbol.code().raw()`
     * @{
     *//*! \cond DOCS_EXCLUDE */
        TABLE refund_t { /*! \endcond */
          /**
           * @brief owner of the funds being unstaked
           * @details PRIMARY KEY of this table is `owner.value`
           */
          name owner; 
          
          /**
           * @brief how much was unstaked / is being refunded 
           */
          asset stake; 
          
          /**
           * @brief time when the unstake first occured and the refund was created
           */ 
          int32_t request_time; 
          
          /*! \cond DOCS_EXCLUDE */
          uint64_t primary_key() const { return owner.value; }
          bool is_empty()    const { return !stake.amount; }
          /*! \endcond */

        }; /** @}*/

    /**
     * @defgroup Delegates_Table Delegates Table
     * @brief This table stores how much delegators have delegated to their delegates
     * @details SCOPE of this table is the delegator's `name.value` 
     * @{
     *//*! \cond DOCS_EXCLUDE */ 
        TABLE delegate_t { /*! \endcond */ 
          /**
           * @brief receiver of the delegated stake
           */ 
          name delegate; 
          
          /**
           * @brief PRIMARY KEY of this table is an unused dummy
           */ 
          uint64_t delegated_id; 

          /**
           * @brief amount of smart tokens staked by delegator to delegate
           */ 
          asset delegated; 
          
          /**
           * @brief SECONDARY KEY of this table is used to find a specific delegate for a specific converter
           * @details uint128_t { delegated.symbol.code().raw() } << 64 ) | delegate.value
           */
          uint128_t by_symbol_delegate() const { return _by_symbl( delegated.symbol.code(), delegate ); } 

          /*! \cond DOCS_EXCLUDE */
          uint64_t primary_key() const { return delegated_id; } 
          bool is_empty() const { return !delegated.amount; }
          /*! \endcond */ 

        }; /** @}*/
  
    /**
     * @brief Only callable by owner of given multi-converter (with associated multi-token), and this multi-staking contract
     * @param refund_delay_sec - deferral period for refund request exeuction
     * @param start_vote_after - duration until weighted median voting updates start getting pushed
     * @param converter - name of multi-converter contract to be tethered to this multi-staking contract
     * @param enabled - is staking/voting enabled globally for every converter in given multi-converter
     */ 
    ACTION setconfig(uint32_t refund_delay_sec, uint32_t start_vote_after, const name& converter, bool enabled);
    
    /**
     * @brief action for delegating stake's voting weight (with the option to transfer ownership)
     * @param amount - amount of converter token being delegated / transfered
     * @param from - owner of converter tokens, who had previously staked them
     * @param to - delegate to whom `from` is delegating / transferring 
     * @param transfer - flag indicating whether ownership of stake is being transfered
     */
    ACTION delegate(const asset& amount, const name& from, const name& to, bool transfer);

    /**
     * @brief action for pulling stake from a delegate while unstaking, or simply unstaking own stake
     * @param from - delegate from whom `to` is undelegating
     * @param to - name of delegator, original stake owne who is undelegating
     * @param unstake - flag indicating whether to transfer out `amount` (unstake via refund request)
     * @param amount - of converter token being undelegated / unstaked
     */ 
    ACTION undelegate(const name& from, const name& to, bool unstake, asset amount);
    
    /**
     * @brief action for triggering pending stake refund for any user if deferred transaction didn't go through
     * @param symbl - symbol of converter given refund is due to
     * @param owner - owner of the original stake, refund request
     */
    ACTION refund(symbol_code symbl, const name& owner);

    /**
     * @brief action for voting on a specific property for a specific converter
     * @param property - name of the property being voted on, e.g. "fee"_n
     * @param value - value of the property voting on, symbol of converter
     * @param owner - name of the voter   
     * @pre vote owner must stake first, prior to voting, or have been delegated to
     */
    ACTION vote(const name& property,  const asset& value, const name& owner);

    /**
     * @brief notification handler for transfers to the staking contract
     * @param from - multi-token contract holding the smart token balances
     * @param to - smart token owner
     * @param quantity - quantity of converter tokens being transferred in
     * @param memo - transfer memo is diregarded here unlike in multi-converter
     */
    [[eosio::on_notify("*::transfer")]]
    void on_transfer(name from, name to, asset quantity, string memo);
    
  private: 
    using transfer_action = action_wrapper<name("transfer"), &MultiStaking::on_transfer>;
    typedef eosio::multi_index<"votes"_n, vote_t, 
                         indexed_by<"bysymbl"_n, 
                           const_mem_fun<vote_t, uint128_t, 
                             &vote_t::by_symbol_property>>> votes;
    typedef eosio::singleton <"config"_n, config_t> configs;
    typedef eosio::multi_index <"config"_n, config_t> dummy;
    typedef eosio::multi_index <"stakes"_n, stake_t> stakes;
    typedef eosio::multi_index <"weights"_n, weight_t> weights;
    typedef eosio::multi_index <"refunds"_n, refund_t> refunds;
    typedef eosio::multi_index <"delegates"_n, delegate_t, 
                         indexed_by<"bysymbl"_n, 
                           const_mem_fun <delegate_t, uint128_t, 
                             &delegate_t::by_symbol_delegate >>> delegation;

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

}; /** @}*/
