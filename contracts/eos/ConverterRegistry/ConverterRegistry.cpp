#include "../Common/common.hpp"
#include "../Token/Token.hpp"
#include "../BancorConverter/BancorConverter.hpp"
#include "ConverterRegistry.hpp"

ACTION ConverterRegistry::addconverter(const converter_t& converter) {
    check(is_converter_active(converter), "converter is inactive");
    converters converters_table(get_self(), converter.pool_token.get_symbol().code().raw());
    pool_tokens pool_tokens_table(get_self(), converter.pool_token.get_symbol().code().raw());
    
    BancorConverter::reserves reserves_table(converter.contract, converter.pool_token.get_symbol().code().raw());
 
    const auto converter_ptr = converters_table.find(converter.pool_token.get_contract().value);
    check(converter_ptr == converters_table.end(), "converter already exists");
    const auto pool_token_ptr = pool_tokens_table.find(converter.pool_token.get_contract().value);
    check(pool_token_ptr == pool_tokens_table.end(), "pool token already exists");

    converters_table.emplace(get_self(), [&converter](converter_t& c){
        c = converter;
    });
    pool_tokens_table.emplace(get_self(), [&converter](token_t& p){
        p.token = converter.pool_token;
    });
    
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

ACTION ConverterRegistry::rmconverter(const converter_t& converter) {
    check(has_auth(get_self()) || !is_converter_active(converter), "cannot remove an active converter");

    converters converters_table(get_self(), converter.pool_token.get_symbol().code().raw());
    const auto converter_ptr = converters_table.require_find(converter.pool_token.get_contract().value, "can't remove an unexisting converter");
    converters_table.erase(converter_ptr);
    
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

    pool_tokens pool_tokens_table(get_self(), converter.pool_token.get_symbol().code().raw());
    const token_t& pool_token = pool_tokens_table.get(converter.pool_token.get_contract().value, "pool token not found");
    pool_tokens_table.erase(pool_token);
}

bool ConverterRegistry::is_converter_active(const converter_t& converter) {
    BancorConverter::converters converters_table(converter.contract, converter.pool_token.get_symbol().code().raw());
    BancorConverter::reserves reserves_table(converter.contract, converter.pool_token.get_symbol().code().raw());
    BancorConverter::settings settings_table(converter.contract, converter.contract.value);
    
    const auto st = settings_table.require_find("settings"_n.value, "cannot find BancorConverter::settings");   
    const auto converter_ptr = converters_table.find(converter.pool_token.get_symbol().code().raw());

    if (converter_ptr == converters_table.end() || reserves_table.begin() == reserves_table.end())
        return false;

    for (const BancorConverter::reserve_t& reserve : reserves_table) {
        if (reserve.balance.amount == 0)
            return false;
    }

    Token::stats stats_table(st->multi_token, converter_ptr->currency.code().raw());
    const Token::currency_stats& stats = stats_table.get(converter.pool_token.get_symbol().code().raw(), "no stats found");
    if (stats.supply.amount == 0)
        return false;
    
    return true;
}

