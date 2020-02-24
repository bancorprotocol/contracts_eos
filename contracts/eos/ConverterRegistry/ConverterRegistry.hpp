
/**
 *  @file
 *  @copyright defined in ../../../LICENSE
 */
#pragma once

#include <eosio/eosio.hpp>
#include <eosio/transaction.hpp>
#include <eosio/asset.hpp>

using namespace eosio;
using namespace std;


CONTRACT ConverterRegistry : public eosio::contract {
    public:
        using contract::contract;
            TABLE token_t {
                extended_symbol token;

                uint64_t primary_key() const { return token.get_contract().value; }
            };
            TABLE converter_t {
                name contract;
                extended_symbol pool_token;

                uint64_t primary_key() const { return pool_token.get_contract().value; }
            };
            TABLE convertible_pair_t {
                uint64_t id;
                extended_symbol from_token;
                extended_symbol to_token;

                uint64_t primary_key() const { return id; }
                uint64_t by_from_token_contract()   const { return from_token.get_contract().value; } // not unique
                // uint128_t by_from_token_contract_and_symbol()   const { return ( uint128_t{ from_token.get_contract().value } << 64 ) | to_token.get_symbol().code().raw(); } // unique
            };

        ACTION addconverter(const converter_t& converter);
        ACTION rmconverter(const converter_t& converter);
        typedef eosio::multi_index<"converters"_n, converter_t> converters;
        typedef eosio::multi_index<"pooltokens"_n, token_t> pool_tokens;
        typedef eosio::multi_index<"cnvrtblpairs"_n, convertible_pair_t,         
            indexed_by<"bycontract"_n, const_mem_fun <convertible_pair_t, uint64_t, &convertible_pair_t::by_from_token_contract >>
        > convertible_pairs;
    private:
        bool is_converter_active(const converter_t& converter);
};
