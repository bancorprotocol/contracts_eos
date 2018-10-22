#include "./BancorConverter.hpp"
#include "../Common/common.hpp"
#include <math.h>

using namespace eosio;

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

ACTION BancorConverter::init(name smart_contract,
                             asset smart_currency,
                             bool  smart_enabled,
                             bool  enabled,
                             name  network,
                             bool  verify_ram,
                             uint64_t max_fee,
                             uint64_t fee) {
    require_auth(_self);
    eosio_assert(fee < 1000, "must be under 1000");

    settings_t new_settings;
    new_settings.smart_contract  = smart_contract;
    new_settings.smart_currency  = smart_currency;
    new_settings.smart_enabled   = smart_enabled;
    new_settings.enabled         = enabled;
    new_settings.network         = network;
    new_settings.verify_ram      = verify_ram;
    new_settings.max_fee         = max_fee;
    new_settings.fee             = fee;

    settings settings_table(_self, _self.value);
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

    settings settings_table(_self, _self.value);
    auto converter_settings = settings_table.get();
    auto current_smart_supply = ((get_supply(converter_settings.smart_contract, converter_settings.smart_currency.symbol.code())).amount + converter_settings.smart_currency.amount) / pow(10, converter_settings.smart_currency.symbol.precision());
    auto reserve_balance = ((get_balance_amount(contract, _self, currency.symbol.code())) + currency.amount) / pow(10, currency.symbol.precision()); 
    EMIT_PRICE_DATA_EVENT(current_smart_supply, contract, currency.symbol.code(), reserve_balance, ratio);
}

void BancorConverter::convert(name from, eosio::asset quantity, std::string memo, name code) {
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount != 0, "zero quantity is disallowed");
    auto from_amount = quantity.amount / pow(10, quantity.symbol.precision());
    auto memo_object = parse_memo(memo);
    eosio_assert(memo_object.path.size() > 2, "invalid memo format");
    settings settings_table(_self, _self.value);
    auto converter_settings = settings_table.get();
    eosio_assert(converter_settings.enabled, "converter is disabled");
    eosio_assert(converter_settings.network == from, "converter can only receive from network contract");

    auto contract_name = name(memo_object.path[0].c_str());
    eosio_assert(contract_name == _self, "wrong converter");
    auto from_path_currency = quantity.symbol.code().raw();
    auto middle_path_currency = symbol_code(memo_object.path[1].c_str()).raw();
    auto to_path_currency = symbol_code(memo_object.path[2].c_str()).raw();
    eosio_assert(from_path_currency != to_path_currency, "cannot convert to self");
    auto smart_symbol_name = converter_settings.smart_currency.symbol.code().raw();
    eosio_assert(middle_path_currency == smart_symbol_name, "must go through the relay token");
    auto from_token = get_reserve(from_path_currency, converter_settings);
    auto to_token = get_reserve(to_path_currency, converter_settings);

    auto from_currency = from_token.currency;
    auto to_currency = to_token.currency;
    
    auto from_contract = from_token.contract;
    auto to_contract = to_token.contract;

    bool incoming_smart_token = (from_currency.symbol.code().raw() == smart_symbol_name);
    bool outgoing_smart_token = (to_currency.symbol.code().raw() == smart_symbol_name);
    
    auto from_ratio = from_token.ratio;
    auto to_ratio = to_token.ratio;

    eosio_assert(from_token.p_enabled, "'from' token purchases disabled");
    eosio_assert(code == from_contract, "unknown 'from' contract");
    auto current_from_balance = ((get_balance(from_contract, _self, from_currency.symbol.code())).amount + from_currency.amount - quantity.amount) / pow(10, from_currency.symbol.precision()); 
    auto current_to_balance = ((get_balance(to_contract, _self, to_currency.symbol.code())).amount + to_currency.amount) / pow(10, to_currency.symbol.precision());
    auto current_smart_supply = ((get_supply(converter_settings.smart_contract, converter_settings.smart_currency.symbol.code())).amount + converter_settings.smart_currency.amount) / pow(10, converter_settings.smart_currency.symbol.precision());

    name final_to = name(memo_object.to_token.c_str());
    double smart_tokens = 0;
    double to_tokens = 0;
    int64_t total_fee_amount = 0;
    bool quick = false;
    if (incoming_smart_token) {
        // destory received token
        action(
            permission_level{ _self, "active"_n },
            converter_settings.smart_contract, "retire"_n,
            std::make_tuple(quantity, std::string("destroy on conversion"))
        ).send();

        smart_tokens = from_amount;
        current_smart_supply -= smart_tokens;
    }
    else if (!incoming_smart_token && !outgoing_smart_token && (from_ratio == to_ratio) && (converter_settings.fee == 0)) {
        to_tokens = quick_convert(current_from_balance, from_amount, current_to_balance);
        quick = true;
    }
    else {
        smart_tokens = calculate_purchase_return(current_from_balance, from_amount, current_smart_supply, from_ratio);
        current_smart_supply += smart_tokens;
        if (converter_settings.fee > 0) {
            double ffee = (1.0 * converter_settings.fee / 1000.0);
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
        to_tokens = smart_tokens;
        issue = true;
    }
    else if (!quick) {
        if (converter_settings.fee) {
            double ffee = (1.0 * converter_settings.fee / 1000.0);
            auto fee = smart_tokens * ffee;
            if (converter_settings.max_fee > 0 && fee > converter_settings.max_fee)
                fee = converter_settings.max_fee;
            int64_t fee_amount = (fee * pow(10, converter_settings.smart_currency.symbol.precision()));
            if (fee_amount > 0) {
                smart_tokens = smart_tokens - fee;
                total_fee_amount += fee_amount;
            }
        }

        to_tokens = calculate_sale_return(current_to_balance, smart_tokens, current_smart_supply, to_ratio);
    }

    int64_t to_amount = (to_tokens * pow(10, to_currency.symbol.precision()));
    EMIT_CONVERSION_EVENT(memo, from_token.contract, from_currency.symbol.code(), to_token.contract, to_currency.symbol.code(), from_amount, to_amount, total_fee_amount);

    if (incoming_smart_token || !outgoing_smart_token)
        EMIT_PRICE_DATA_EVENT(current_smart_supply, to_token.contract, to_currency.symbol.code(), current_to_balance - to_amount, to_ratio);
    if (outgoing_smart_token || !incoming_smart_token)
        EMIT_PRICE_DATA_EVENT(current_smart_supply, from_token.contract, from_currency.symbol.code(), current_from_balance, from_ratio);

    auto next_hop_memo = next_hop(memo_object);
    auto new_memo = build_memo(next_hop_memo);
    auto new_asset = asset(to_amount, to_currency.symbol);
    name inner_to = converter_settings.network;
    if (next_hop_memo.path.size() == 0) {
        inner_to = final_to;
        verify_min_return(new_asset, memo_object.min_return);
        if (converter_settings.verify_ram)
            verify_entry(inner_to, to_contract, new_asset);
        new_memo = std::string("convert");
    }

    if (issue)
        action(
            permission_level{ _self, "active"_n },
            to_contract, "issue"_n,
            std::make_tuple(inner_to, new_asset, new_memo) 
        ).send();
    else
        action(
            permission_level{ _self, "active"_n },
            to_contract, "transfer"_n,
            std::make_tuple(_self, inner_to, new_asset, new_memo)
        ).send();
}

 // returns a reserve object
 // can also be called for the smart token itself
const BancorConverter::reserve_t& BancorConverter::get_reserve(uint64_t name, const settings_t& settings) {
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

// returns the balance object for an account
asset BancorConverter::get_balance(name contract, name owner, symbol_code sym) {
    accounts accountstable(contract, owner.value);
    const auto& ac = accountstable.get(sym.raw());
    return ac.balance;
}

// returns the balance amount for an account
uint64_t BancorConverter::get_balance_amount(name contract, name owner, symbol_code sym) {
    accounts accountstable(contract, owner.value);

    auto ac = accountstable.find(sym.raw());
    if (ac != accountstable.end())
        return ac->balance.amount;

    return 0;
}

// returns a token supply
asset BancorConverter::get_supply(name contract, symbol_code sym) {
    stats statstable(contract, sym.raw());
    const auto& st = statstable.get(sym.raw());
    return st.supply;
}

// asserts if the supplied account doesn't have an entry for a given token
void BancorConverter::verify_entry(name account, name currency_contact, eosio::asset currency) {
    accounts accountstable(currency_contact, account.value);
    auto ac = accountstable.find(currency.symbol.code().raw());
    eosio_assert(ac != accountstable.end(), "must have entry for token (claim token first)");
}

// asserts if a conversion resulted in an amount lower than the minimum amount defined by the caller
void BancorConverter::verify_min_return(eosio::asset quantity, std::string min_return) {
	float ret = stof(min_return.c_str());
    int64_t ret_amount = (ret * pow(10, quantity.symbol.precision()));
    eosio_assert(quantity.amount >= ret_amount, "below min return");
}

// given a token supply, reserve balance, ratio and a input amount (in the reserve token),
// calculates the return for a given conversion (in the main token)
double BancorConverter::calculate_purchase_return(double balance, double deposit_amount, double supply, int64_t ratio) {
    double R(supply);
    double C(balance + deposit_amount);
    double F(ratio / 1000.0);
    double T(deposit_amount);
    double ONE(1.0);

    double E = -R * (ONE - pow(ONE + T / C, F));
    return E;
}

// given a token supply, reserve balance, ratio and a input amount (in the main token),
// calculates the return for a given conversion (in the reserve token)
double BancorConverter::calculate_sale_return(double balance, double sell_amount, double supply, int64_t ratio) {
    double R(supply - sell_amount);
    double C(balance);
    double F(1000.0 / ratio);
    double E(sell_amount);
    double ONE(1.0);

    double T = C * (pow(ONE + E/R, F) - ONE);
    return T;
}

double BancorConverter::quick_convert(double balance, double in, double toBalance) {
    return in / (balance + in) * toBalance;
}

float BancorConverter::stof(const char* s) {
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

void BancorConverter::transfer(name from, name to, asset quantity, string memo) {
    if (from == _self) {
        // TODO: prevent withdrawal of funds
        return;
    }

    if (to != _self) 
        return;

    if (memo == "setup") {
        // TODO: emit price data event
        return;
    }

    convert(from, quantity, memo, _code); 
}

extern "C" {
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        if (action == "transfer"_n.value && code != receiver) {
            eosio::execute_action(eosio::name(receiver), eosio::name(code), &BancorConverter::transfer);
        }
        if (code == receiver) {
            switch (action) { 
                EOSIO_DISPATCH_HELPER(BancorConverter, (init)(setreserve)) 
            }    
        }
        eosio_exit(0);
    }
}
