#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/symbol.hpp>
#include "../Common/common.hpp"
using namespace eosio;
using std::string;
using std::vector;
#define EMIT_CONVERT_EVENT(memo, currentBaseBalance, currentTargetBalance, currentSmartSupply, baseAmount, smartTokens, targetAmount, baseWeight, targetWeight, toConnectorMaxfee,fromConnectorMaxfee, toConnectorFee, fromConnectorFee) \
  START_EVENT("convert", "1.1") \
  EVENTKV("memo",memo) \
  EVENTKV("currentBaseBalance",currentBaseBalance) \
  EVENTKV("currentTargetBalance",currentTargetBalance) \
  EVENTKV("currentSmartSupply",currentSmartSupply) \
  EVENTKV("baseAmount",baseAmount) \
  EVENTKV("smartTokens",smartTokens) \
  EVENTKV("targetAmount",targetAmount) \
  EVENTKV("baseWeight",baseWeight) \
  EVENTKV("targetWeight",targetWeight) \
  EVENTKV("toConnectorMaxfee",toConnectorMaxfee) \
  EVENTKV("fromConnectorMaxfee",fromConnectorMaxfee) \
  EVENTKV("toConnectorFee",toConnectorFee) \
  EVENTKV("currentTargetBalance",currentTargetBalance) \
  EVENTKVL("fromConnectorFee", fromConnectorFee) \
  END_EVENT()

CONTRACT BancorConverter : public eosio::contract {
    using contract::contract;
    public:

        TABLE connector_t {
            name contract;
            asset        currency;
            uint64_t     weight;
            bool         enabled;
            uint64_t     fee;
            name feeaccount;
            uint64_t     maxfee;        
            uint64_t     primary_key() const { return currency.symbol.code().raw(); }
        };
    

        TABLE cstate_t {
            name manager;
            name smart_contract;
            asset        smart_currency;
            bool         smart_enabled;
            bool         enabled;
            name network;
            bool         verifyram;
            uint64_t primary_key() const { return manager.value; }
        };


 
        typedef eosio::multi_index<"cstate"_n, cstate_t> converters;
        typedef eosio::multi_index<"connector"_n, connector_t> connectors;

        void transfer(name from, name to, asset quantity, string memo);

        ACTION create(name smart_contract,
                        asset        smart_currency,
                        bool         smart_enabled,
                        bool         enabled,
                        name network,
                        bool         verifyram);
        
        ACTION setconnector(name contract,
                            asset        currency,
                            uint64_t     weight,
                            bool         enabled,
                            uint64_t     fee,
                            name feeaccount,
                            uint64_t     maxfee );
    private:
        void convert( name from, eosio::asset quantity, std::string memo, name code);
        const connector_t& lookupConnector(uint64_t name, cstate_t state );
        
};
