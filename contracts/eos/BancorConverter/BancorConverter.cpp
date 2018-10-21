#include "./BancorConverter.hpp"
#include "../Common/common.hpp"
#include <math.h>

using namespace eosio;
typedef double real_type;

struct account {
    asset    balance;
    uint64_t primary_key()const { return balance.symbol.code().raw(); }
};

TABLE currency_stats {
    asset   supply;
    asset   max_supply;
    name    issuer;
    uint64_t primary_key() const { return supply.symbol.code().raw(); }
};

typedef eosio::multi_index<"stat"_n, currency_stats> stats;
typedef eosio::multi_index<"accounts"_n, account> accounts;

asset get_balance(name contract, name owner, symbol_code sym) {
    accounts accountstable(contract, owner.value);
    const auto& ac = accountstable.get(sym.raw());
    return ac.balance;
}      

asset get_supply(name contract, symbol_code sym) {
    stats statstable(contract, sym.raw());
    const auto& st = statstable.get(sym.raw());
    return st.supply;
}

real_type calculate_purchase_return(real_type balance, real_type deposit_amount, real_type supply, int64_t ratio) {
    real_type R(supply);
    real_type C(balance + deposit_amount);
    real_type F(ratio / 1000.0);
    real_type T(deposit_amount);
    real_type ONE(1.0);

    real_type E = -R * (ONE - pow(ONE + T / C, F));
    return E;
}

real_type calculate_sale_return(real_type balance, real_type sell_amount, real_type supply, int64_t ratio) {
    real_type R(supply - sell_amount);
    real_type C(balance);
    real_type F(1000.0 / ratio);
    real_type E(sell_amount);
    real_type ONE(1.0);

    real_type T = C * (pow(ONE + E/R, F) - ONE);
    return T;
}

real_type quick_convert(real_type balance, real_type in, real_type targetBalance) {
    return in / (balance + in) * targetBalance;
}
   
float stof(const char* s) {
    float rez = 0, fact = 1;
    if (*s == '-') {
        s++;
        fact = -1;
    }

    for (int point_seen = 0; *s; s++) {
        if (*s == '.') {
            point_seen = 1; 
            continue;
        }

        int d = *s - '0';
        if (d >= 0 && d <= 9) {
            if (point_seen) fact /= 10.0f;
            rez = rez * 10.0f + (float)d;
        }
    }

    return rez * fact;
};

void verify_entry(name account, name currency_contact, eosio::asset currency) {
    accounts accountstable(currency_contact, account.value);
    auto ac = accountstable.find(currency.symbol.code().raw());
    eosio_assert(ac != accountstable.end() , "must have entry for token (claim token first)");
}

void verify_min_return(eosio::asset quantity, std::string min_return) {
	float ret = stof(min_return.c_str());
    int64_t ret_amount = (ret * pow(10, quantity.symbol.precision()));
    eosio_assert(quantity.amount >= ret_amount, "below min return");
}

const BancorConverter::reserve_t& BancorConverter::lookup_reserve(uint64_t name, const settingstype& settings) {
    if (settings.smart_currency.symbol.code().raw() == name) {
        static reserve_t temp_reserve;
        temp_reserve.ratio = 0;
        temp_reserve.contract = settings.smart_contract;
        temp_reserve.currency = settings.smart_currency;
        temp_reserve.p_enabled = settings.smart_enabled;
        return temp_reserve;
    }

    reserves reserves_table(_self, _self.value);
    auto existing = reserves_table.find(name);
    eosio_assert(existing != reserves_table.end(), "reserve not found");
    return *existing;
}

void BancorConverter::convert(name from, eosio::asset quantity, std::string memo, name code) {
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount != 0, "zero quantity is disallowed");
    auto base_amount = quantity.amount / pow(10, quantity.symbol.precision());
    auto memo_object = parse_memo(memo);
    eosio_assert(memo_object.path.size() > 2, "invalid memo format");
    settings settings_table(_self, _self.value);
    auto converter_settings = settings_table.get();
    eosio_assert(converter_settings.enabled, "converter is disabled");
    eosio_assert(converter_settings.network == from, "converter can only receive from network contract");

    auto contract_name = name(memo_object.path[0].c_str());
    eosio_assert(contract_name == _self, "wrong converter");
    auto base_path_currency = quantity.symbol.code().raw();
    auto middle_path_currency = symbol_code(memo_object.path[1].c_str()).raw();
    auto target_path_currency = symbol_code(memo_object.path[2].c_str()).raw();
    eosio_assert(base_path_currency != target_path_currency, "cannot convert to self");
    auto smart_symbol_name = converter_settings.smart_currency.symbol.code().raw();
    eosio_assert(middle_path_currency == smart_symbol_name, "must go through the relay token");
    auto from_reserve =  lookup_reserve(base_path_currency, converter_settings);
    auto to_reserve = lookup_reserve(target_path_currency, converter_settings);

    auto base_currency = from_reserve.currency;
    auto target_currency = to_reserve.currency;
    
    auto target_contract = to_reserve.contract;
    auto base_contract = from_reserve.contract;

    bool incoming_smart_token = (base_currency.symbol.code().raw() == smart_symbol_name);
    bool outgoing_smart_token = (target_currency.symbol.code().raw() == smart_symbol_name);
    
    auto base_ratio = from_reserve.ratio;
    auto target_ratio = to_reserve.ratio;

    eosio_assert(from_reserve.p_enabled, "'from' reserve purchases disabled");
    eosio_assert(code == base_contract, "unknown 'from' contract");
    auto current_base_balance = ((get_balance(base_contract, _self, base_currency.symbol.code())).amount + base_currency.amount - quantity.amount) / pow(10, base_currency.symbol.precision()); 
    auto current_target_balance = ((get_balance(target_contract, _self, target_currency.symbol.code())).amount + target_currency.amount) / pow(10, target_currency.symbol.precision());
    auto current_smart_supply = ((get_supply(converter_settings.smart_contract, converter_settings.smart_currency.symbol.code())).amount + converter_settings.smart_currency.amount)  / pow(10, converter_settings.smart_currency.symbol.precision());

    name final_to = name(memo_object.target.c_str());
    real_type smart_tokens = 0;
    real_type target_tokens = 0;
    int64_t total_fee_amount = 0;
    bool quick = false;
    if (incoming_smart_token) {
        // destory received token
        action(
            permission_level{ _self, "active"_n },
            converter_settings.smart_contract, "retire"_n,
            std::make_tuple(quantity, std::string("destroy on conversion"))
        ).send();

        smart_tokens = base_amount;
        current_smart_supply -= smart_tokens;
    }
    else if (!incoming_smart_token && !outgoing_smart_token && (base_ratio == target_ratio) && (converter_settings.fee == 0)) {
        target_tokens = quick_convert(current_base_balance, base_amount, current_target_balance);    
        quick = true;
    }
    else {
        smart_tokens = calculate_purchase_return(current_base_balance, base_amount, current_smart_supply, base_ratio);
        current_smart_supply += smart_tokens;
        if (converter_settings.fee > 0) {
            real_type ffee = (1.0 * converter_settings.fee / 1000.0);
            auto fee = smart_tokens * ffee;
            if (converter_settings.max_fee > 0 && fee > converter_settings.max_fee)
                fee = converter_settings.max_fee;

            int64_t fee_amount = (fee * pow(10, converter_settings.smart_currency.symbol.precision()));
            if (fee_amount > 0) {
                smart_tokens = smart_tokens - fee;
                total_fee_amount += fee_amount;
            }
        }
    }

    auto issue = false;
    if (outgoing_smart_token) {
        eosio_assert(memo_object.path.size() == 3, "smart token must be final currency");
        target_tokens = smart_tokens;
        issue = true;
    }
    else if (!quick) {
        if (converter_settings.fee) {
            real_type ffee = (1.0 * converter_settings.fee / 1000.0);
            auto fee = smart_tokens * ffee;
            if (converter_settings.max_fee > 0 && fee > converter_settings.max_fee)
                fee = converter_settings.max_fee;
            int64_t fee_amount = (fee * pow(10, converter_settings.smart_currency.symbol.precision()));
            if (fee_amount > 0) {
                smart_tokens = smart_tokens - fee;
                total_fee_amount += fee_amount;
            }
        }

        target_tokens = calculate_sale_return(current_target_balance, smart_tokens, current_smart_supply, target_ratio);    
    }

    int64_t target_amount = (target_tokens * pow(10, target_currency.symbol.precision()));
    EMIT_CONVERSION_EVENT(memo, current_base_balance, current_target_balance, current_smart_supply, base_amount, smart_tokens, target_amount, base_ratio, target_ratio, total_fee_amount)

    auto next_hop_memo = next_hop(memo_object);
    auto new_memo = build_memo(next_hop_memo);
    auto new_asset = asset(target_amount,target_currency.symbol);
    name inner_to = converter_settings.network;
    if (next_hop_memo.path.size() == 0) {
        inner_to = final_to;
        verify_min_return(new_asset, memo_object.min_return);
        if (converter_settings.verify_ram)
            verify_entry(inner_to, target_contract ,new_asset);
        new_memo = std::string("convert");
    }

    if (issue)
        action(
            permission_level{ _self, "active"_n },
            target_contract, "issue"_n,
            std::make_tuple(inner_to, new_asset, new_memo) 
        ).send();
    else
        action(
            permission_level{ _self, "active"_n },
            target_contract, "transfer"_n,
            std::make_tuple(_self, inner_to, new_asset, new_memo)
        ).send();
}

void BancorConverter::transfer(name from, name to, asset quantity, string memo) {
    if (from == _self) {
        // TODO: prevent withdraw of funds
        return;
    }

    if (to != _self || memo == "initial") 
        return;

    convert(from , quantity, memo, _code); 
}

ACTION BancorConverter::create(name smart_contract,
                               asset smart_currency,
                               bool  smart_enabled,
                               bool  enabled,
                               name  network,
                               bool  verify_ram,
                               uint64_t max_fee,
                               uint64_t fee) {
    require_auth(_self);
    eosio_assert(fee < 1000, "must be under 1000");

    settings settings_table(_self, _self.value);
    settingstype new_settings;
    new_settings.smart_contract  = smart_contract;
    new_settings.smart_currency  = smart_currency;
    new_settings.smart_enabled   = smart_enabled;
    new_settings.enabled         = enabled;
    new_settings.network         = network;
    new_settings.verify_ram      = verify_ram;
    new_settings.max_fee         = max_fee;
    new_settings.fee             = fee;
    settings_table.set(new_settings, _self);
}

ACTION BancorConverter::setreserve(name contract,
                                   asset    currency,
                                   uint64_t ratio,
                                   bool     p_enabled)
{
    require_auth(_self);   

    reserves reserves_table(_self, _self.value);
    auto existing = reserves_table.find(currency.symbol.code().raw());
    if (existing != reserves_table.end()) {
      reserves_table.modify(existing, _self, [&](auto& s) {
          s.contract    = contract;
          s.currency    = currency;
          s.ratio       = ratio;
          s.p_enabled   = p_enabled;
      });
    }
    else reserves_table.emplace(_self, [&](auto& s) {
        s.contract    = contract;
        s.currency    = currency;
        s.ratio       = ratio;
        s.p_enabled   = p_enabled;
    });
}

extern "C" {
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        if (action == "transfer"_n.value && code != receiver) {
            eosio::execute_action(eosio::name(receiver), eosio::name(code), &BancorConverter::transfer);
        }
        if (code == receiver) {
            switch (action) { 
                EOSIO_DISPATCH_HELPER(BancorConverter, (create)(setreserve)) 
            }    
        }
        eosio_exit(0);
    }
}
