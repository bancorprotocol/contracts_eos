
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
        struct Converter {
            name contract;
            extended_symbol pool_token;
        };

        TABLE token_t {
            uint64_t id;
            extended_symbol token;

            uint64_t primary_key() const { return id; }
            
            uint128_t by_token() const {
                return encode_name_and_symcode(token.get_contract(), token.get_symbol().code());
            }
        };
        TABLE converter_t {
            uint64_t id;
            Converter converter;

            uint64_t primary_key() const { return id; }
            
            uint128_t by_converter_contract_and_pool_token() const {
                return encode_name_and_symcode(converter.contract, converter.pool_token.get_symbol().code());
            }
            
        };
        TABLE convertible_pair_t {
            uint64_t id;
            extended_symbol from_token;
            extended_symbol to_token;
            Converter converter;

            uint64_t primary_key() const { return id; }
            uint64_t by_from_token_contract()   const { return from_token.get_contract().value; } // not unique
            // uint128_t by_from_token_contract_and_symbol()   const { return ( uint128_t{ from_token.get_contract().value } << 64 ) | to_token.get_symbol().code().raw(); } // unique
        };

        inline BancorConverterRegistry(name receiver, name code, datastream<const char *> ds);

        ACTION addconverter(const name& converter_account, const symbol_code& pool_token_code);
        ACTION rmconverter(const name& converter_account, const symbol_code& pool_token_code);

        static uint128_t encode_name_and_symcode(name contract, symbol_code pool_token) {
            return ( uint128_t{ contract.value } << 64 ) | pool_token.raw();
        }
        typedef eosio::multi_index<"converters"_n, converter_t,
            indexed_by<"contract.sym"_n, const_mem_fun <converter_t, uint128_t, &converter_t::by_converter_contract_and_pool_token >>
        > converters;
        typedef eosio::multi_index<"liqdtypools"_n, converter_t,
            indexed_by<"contract.sym"_n, const_mem_fun <converter_t, uint128_t, &converter_t::by_converter_contract_and_pool_token >>
        > liquidity_pools;
        typedef eosio::multi_index<"pooltokens"_n, token_t,
            indexed_by<"token"_n, const_mem_fun <token_t, uint128_t, &token_t::by_token >>
        > pool_tokens;
        typedef eosio::multi_index<"cnvrtblpairs"_n, convertible_pair_t,         
            indexed_by<"bycontract"_n, const_mem_fun <convertible_pair_t, uint64_t, &convertible_pair_t::by_from_token_contract >>
        > convertible_pairs;
    private:
        converters converters_table;
        liquidity_pools liquidity_pools_table;
        pool_tokens pool_tokens_table;

        void register_converter(const Converter& converter);
        void register_liquidity_pool(const Converter& liquidity_pool);
        void register_convertible_pairs(const Converter& converter);
        void register_pool_token(const extended_symbol& pool_token);
        void unregister_converter(const Converter& converter);
        void unregister_liquidity_pool(const Converter& liquidity_pool);
        void unregister_pool_token(const extended_symbol& pool_token_sym);
        void unregister_convertible_pairs(const Converter& converter);
        bool is_converter_active(const Converter& converter);

        const converters::const_iterator find_converter(const name& converter_contract, const symbol_code& pool_token);
        const liquidity_pools::const_iterator find_liquidity_pool(const name& liquidity_pool_contract, const symbol_code& pool_token);
        const pool_tokens::const_iterator find_pool_token(const extended_symbol& token);

        // Utils
        const extended_symbol get_converter_pool_token(const name& converter_account, const symbol_code& pool_token);
        const BancorConverter::settings_t& get_converter_global_settings(const name& converter);
};
