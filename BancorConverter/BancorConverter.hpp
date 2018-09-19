#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/types.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/symbol.hpp>
#include <eosiolib/currency.hpp>
#include <eosio.system/eosio.system.hpp>

using namespace eosio;



class BancorConverter : public eosio::contract {
    
    
  
   public:
    // @abi table 
    struct connector {
        account_name contract;
        asset        currency;
        uint64_t     weight;
        bool         enabled;
        uint64_t     fee;
        account_name feeaccount;
        uint64_t        maxfee;        
        uint64_t primary_key() const { return currency.symbol.name(); }
        EOSLIB_SERIALIZE( connector, (contract)(currency)(weight)(enabled)(fee)(feeaccount)(maxfee) )
    };
    
    // @abi table
    struct cstate
    {
       account_name manager;
       account_name smart_contract;
       asset        smart_currency;
       bool         smart_enabled;
       bool         enabled;
       account_name network;
       bool         verifyram;
       uint64_t primary_key() const { return manager; }
       EOSLIB_SERIALIZE( cstate, (manager)(smart_contract)(smart_currency)(smart_enabled)(enabled)(network)(verifyram) )
    };
 
    typedef eosio::multi_index<N(cstate), cstate> converters;
    typedef eosio::multi_index<N(connector), connector> connectors;
    
      BancorConverter( account_name self )
       :contract(self),
       _self(self),
       cstates( self, self ),
       connectors_table( self, self )
      {}
      
      void apply( account_name contract, account_name act );
      // @abi action
      struct create
      {
          account_name smart_contract;
          asset        smart_currency;
          bool         smart_enabled;
          bool         enabled;
          account_name network;
          bool         verifyram;
          EOSLIB_SERIALIZE( create, (smart_contract)(smart_currency)(smart_enabled)(enabled)(network)(verifyram) )
      };
      
      // @abi action
      struct setconnector
      {
        account_name contract;
        asset        currency;
        uint64_t     weight;
        bool         enabled;
        uint64_t     fee;
        account_name feeaccount;
        uint64_t     maxfee;
        EOSLIB_SERIALIZE( setconnector, (contract)(currency)(weight)(enabled)(fee)(feeaccount)(maxfee) )
      };
   private:
      account_name      _self;
      converters        cstates;
      connectors        connectors_table;
      const connector& lookupConnector(uint64_t name, cstate state );
      void on( const currency::transfer& t, account_name code );
      void convert( account_name from, eosio::asset quantity, std::string memo, account_name code);
      void oncreate(const create& sc, account_name code );
      void onsetconnector(const setconnector& sc, account_name code );
};