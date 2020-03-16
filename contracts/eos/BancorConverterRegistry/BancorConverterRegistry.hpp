
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


CONTRACT BancorConverterRegistry : public eosio::contract {
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
                converter_t converter;

                uint64_t primary_key() const { return id; }
                uint64_t by_from_token_contract()   const { return from_token.get_contract().value; } // not unique
                // uint128_t by_from_token_contract_and_symbol()   const { return ( uint128_t{ from_token.get_contract().value } << 64 ) | to_token.get_symbol().code().raw(); } // unique
            };

        ACTION addconverter(const name& converter_account, const symbol_code& pool_token_code);
        ACTION rmconverter(const name& converter_account, const symbol_code& pool_token_code);
        typedef eosio::multi_index<"converters"_n, converter_t> converters;
        typedef eosio::multi_index<"liqdtypools"_n, converter_t> liquidity_pools;
        typedef eosio::multi_index<"pooltokens"_n, token_t> pool_tokens;
        typedef eosio::multi_index<"cnvrtblpairs"_n, convertible_pair_t,         
            indexed_by<"bycontract"_n, const_mem_fun <convertible_pair_t, uint64_t, &convertible_pair_t::by_from_token_contract >>
        > convertible_pairs;
    private:
        void register_converter(const converter_t& converter);
        void register_liquidity_pool(const converter_t& liquidity_pool);
        void register_convertible_pairs(const converter_t& converter);
        void register_pool_token(const extended_symbol& pool_token);
        void unregister_converter(const converter_t& converter);
        void unregister_liquidity_pool(const converter_t& converter);
        void unregister_pool_token(const extended_symbol& pool_token_sym);
        void unregister_convertible_pairs(const converter_t& converter);
        bool is_converter_active(const converter_t& converter);

        // Utils
        const extended_symbol get_converter_pool_token(const name& converter_account, const symbol_code& pool_token);
        const BancorConverter::settings_t& get_converter_global_settings(const name& converter);
};
