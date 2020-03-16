#include "../Common/common.hpp"
#include "../Token/Token.hpp"
#include "../BancorConverter/BancorConverter.hpp"
#include "ConverterRegistry.hpp"

ACTION ConverterRegistry::addconverter(const name& converter_account, const symbol_code& pool_token_code) {
    const extended_symbol pool_token = get_converter_pool_token(converter_account, pool_token_code);
    const converter_t converter = converter_t{ converter_account, pool_token };
    check(is_converter_active(converter), "converter is inactive");

    register_converter(converter);
    register_pool_token(converter.pool_token);
    register_convertible_pairs(converter);

    BancorConverter::reserves reserves_table(converter.contract, converter.pool_token.get_symbol().code().raw());
    uint64_t num_of_reserves = std::distance(reserves_table.cbegin(), reserves_table.cend());
    if (num_of_reserves > 1)
        register_liquidity_pool(converter);
}

ACTION ConverterRegistry::rmconverter(const name& converter_account, const symbol_code& pool_token_code) {
    const extended_symbol pool_token = get_converter_pool_token(converter_account, pool_token_code);
    const converter_t converter = converter_t{ converter_account, pool_token };
    check(has_auth(get_self()) || !is_converter_active(converter), "cannot remove an active converter");

    unregister_converter(converter);
    unregister_pool_token(converter.pool_token);
    unregister_convertible_pairs(converter);
    unregister_liquidity_pool(converter);
}

void ConverterRegistry::register_converter(const converter_t& converter) {
    converters converters_table(get_self(), converter.pool_token.get_symbol().code().raw());
    const auto p_converter = converters_table.find(converter.pool_token.get_contract().value);
    check(p_converter == converters_table.end(), "converter already exists");
    converters_table.emplace(get_self(), [&converter](converter_t& c){
        c = converter;
    });
}

void ConverterRegistry::register_liquidity_pool(const converter_t& liquidity_pool) {
    liquidity_pools liquidity_pools_table(get_self(), liquidity_pool.pool_token.get_symbol().code().raw());
    const auto p_liquidity_pool = liquidity_pools_table.find(liquidity_pool.pool_token.get_contract().value);
    check(p_liquidity_pool == liquidity_pools_table.end(), "liquidity pool already exists");
    liquidity_pools_table.emplace(get_self(), [&liquidity_pool](converter_t& lp){
        lp = liquidity_pool;
    });
}

void ConverterRegistry::register_pool_token(const extended_symbol& pool_token) {
    pool_tokens pool_tokens_table(get_self(), pool_token.get_symbol().code().raw());
    const auto p_pool_token = pool_tokens_table.find(pool_token.get_contract().value);
    check(p_pool_token == pool_tokens_table.end(), "pool token already exists");
    pool_tokens_table.emplace(get_self(), [&pool_token](token_t& p){
        p.token = pool_token;
    });
}

void ConverterRegistry::register_convertible_pairs(const converter_t& converter) {
    BancorConverter::reserves reserves_table(converter.contract, converter.pool_token.get_symbol().code().raw());

    convertible_pairs pool_token_convertible_pairs_table(get_self(), converter.pool_token.get_symbol().code().raw());
    for (const BancorConverter::reserve_t& reserve : reserves_table) {
        convertible_pairs reserve_token_convertible_pairs_table(get_self(), reserve.balance.symbol.code().raw());
        pool_token_convertible_pairs_table.emplace(get_self(), [&](convertible_pair_t& cp){
            cp.id = pool_token_convertible_pairs_table.available_primary_key();
            cp.from_token = extended_symbol(converter.pool_token.get_symbol(), converter.pool_token.get_contract());
            cp.to_token = extended_symbol(reserve.balance.symbol, reserve.contract);
            cp.converter = converter;
        });
        reserve_token_convertible_pairs_table.emplace(get_self(), [&](convertible_pair_t& cp){
            cp.id = reserve_token_convertible_pairs_table.available_primary_key();
            cp.from_token = extended_symbol(reserve.balance.symbol, reserve.contract);
            cp.to_token = extended_symbol(converter.pool_token.get_symbol(), converter.pool_token.get_contract());
            cp.converter = converter;
        });

        for (const BancorConverter::reserve_t& other_reserve : reserves_table) {
            if (other_reserve.contract == reserve.contract && other_reserve.balance.symbol == reserve.balance.symbol)
                continue;
            reserve_token_convertible_pairs_table.emplace(get_self(), [&](convertible_pair_t& cp){
                cp.id = reserve_token_convertible_pairs_table.available_primary_key();
                cp.from_token = extended_symbol(reserve.balance.symbol, reserve.contract);
                cp.to_token = extended_symbol(other_reserve.balance.symbol, other_reserve.contract);
                cp.converter = converter;
            });
        }
    }
}

void ConverterRegistry::unregister_converter(const converter_t& converter) {
    converters converters_table(get_self(), converter.pool_token.get_symbol().code().raw());
    const auto p_converter = converters_table.require_find(converter.pool_token.get_contract().value, "can't remove an unexisting converter");
    converters_table.erase(p_converter);
}

void ConverterRegistry::unregister_liquidity_pool(const converter_t& converter) {
    liquidity_pools liquidity_pools_table(get_self(), converter.pool_token.get_symbol().code().raw());
    const auto p_liquidity_pool = liquidity_pools_table.find(converter.pool_token.get_contract().value);
    if (p_liquidity_pool != liquidity_pools_table.end())
        liquidity_pools_table.erase(p_liquidity_pool);
}

void ConverterRegistry::unregister_pool_token(const extended_symbol& pool_token_sym) {
    pool_tokens pool_tokens_table(get_self(), pool_token_sym.get_symbol().code().raw());
    const token_t& pool_token = pool_tokens_table.get(pool_token_sym.get_contract().value, "pool token not found");
    pool_tokens_table.erase(pool_token);
}

void ConverterRegistry::unregister_convertible_pairs(const converter_t& converter) {
    convertible_pairs pool_token_convertible_pairs_table(get_self(), converter.pool_token.get_symbol().code().raw());
    auto pool_token_index = pool_token_convertible_pairs_table.get_index<"bycontract"_n >();
    auto pool_tokens_pairs_itr = pool_token_index.find(converter.pool_token.get_contract().value);
    while (pool_tokens_pairs_itr != pool_token_index.end()) {
        convertible_pairs reserve_token_convertible_pairs_table(get_self(), pool_tokens_pairs_itr->to_token.get_symbol().code().raw());
        auto reserve_index = reserve_token_convertible_pairs_table.get_index<"bycontract"_n >();
        auto reserve_tokens_pairs_itr = reserve_index.find(pool_tokens_pairs_itr->to_token.get_contract().value);
        while (reserve_tokens_pairs_itr != reserve_index.end()) {
            reserve_tokens_pairs_itr = reserve_index.erase(reserve_tokens_pairs_itr);
        }

        pool_tokens_pairs_itr = pool_token_index.erase(pool_tokens_pairs_itr);
    }
}

bool ConverterRegistry::is_converter_active(const converter_t& converter) {
    BancorConverter::converters converters_table(converter.contract, converter.pool_token.get_symbol().code().raw());
    BancorConverter::reserves reserves_table(converter.contract, converter.pool_token.get_symbol().code().raw());

    const auto p_converter = converters_table.find(converter.pool_token.get_symbol().code().raw());

    if (p_converter == converters_table.end() || reserves_table.begin() == reserves_table.end())
        return false;

    for (const BancorConverter::reserve_t& reserve : reserves_table) {
        if (reserve.balance.amount == 0)
            return false;
    }

    const BancorConverter::settings_t converter_settings = get_converter_global_settings(converter.contract);
    Token::stats stats_table(converter_settings.multi_token, p_converter->currency.code().raw());
    const Token::currency_stats& stats = stats_table.get(converter.pool_token.get_symbol().code().raw(), "no stats found");
    if (stats.supply.amount == 0)
        return false;
    
    return true;
}


// Utils

const extended_symbol ConverterRegistry::get_converter_pool_token(const name& converter_account, const symbol_code& pool_token) {
    BancorConverter::converters converters_table(converter_account, pool_token.raw());
    const BancorConverter::converter_t& converter = converters_table.get(pool_token.raw(), "cannot get converter pool token");
    const BancorConverter::settings_t converter_settings = get_converter_global_settings(converter_account);
    
    return extended_symbol(converter.currency, converter_settings.multi_token);
}

const BancorConverter::settings_t& ConverterRegistry::get_converter_global_settings(const name& converter) {
    BancorConverter::settings settings_table(converter, converter.value);
    const BancorConverter::settings_t& st = settings_table.get("settings"_n.value, "cannot get converter global settings");
    
    return st;
}