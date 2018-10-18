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

real_type calculate_purchase_return(real_type balance, real_type deposit_amount, real_type supply, int64_t weight) {
    real_type R(supply);
    real_type C(balance + deposit_amount);
    real_type F(weight / 1000.0);
    real_type T(deposit_amount);
    real_type ONE(1.0);

    real_type E = -R * (ONE - pow(ONE + T / C, F));
    return E;
}

real_type calculate_sale_return(real_type balance, real_type sell_amount, real_type supply, int64_t weight) {
    real_type R(supply - sell_amount);
    real_type C(balance);
    real_type F(1000.0 / weight);
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
    eosio_assert(ac != accountstable.end() , "must have entry for token. (claim token first)");
}

void verify_min_return(eosio::asset quantity, std::string min_return) {
	float ret = stof(min_return.c_str());
    int64_t ret_amount = (ret * pow(10, quantity.symbol.precision()));
    eosio_assert(quantity.amount >= ret_amount, "below min return");
}

const BancorConverter::connector_t& BancorConverter::lookup_connector(uint64_t name, cstate_t state) {
    if (state.smart_currency.symbol.code().raw() == name) {
        static connector_t temp_connector;
        temp_connector.weight = 0;
        temp_connector.fee = 0;
        temp_connector.contract = state.smart_contract;
        temp_connector.currency = state.smart_currency;
        temp_connector.enabled = state.smart_enabled;
        return temp_connector;
    }
    
    connectors connectors_table(_self, _self.value);
    auto existing = connectors_table.find(name);
    eosio_assert(existing != connectors_table.end(), "connector not found");
    return *existing;
}

void BancorConverter::convert(name from, eosio::asset quantity, std::string memo, name code) {
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount != 0, "zero quantity is disallowed");
    auto base_amount = quantity.amount / pow(10,quantity.symbol.precision());
    auto memo_object = parse_memo(memo);
    eosio_assert(memo_object.path.size() > 2, "invalid memo format");
    converters cstates(_self, _self.value);
    auto converter_state_ptr = cstates.find(_self.value);
    eosio_assert(converter_state_ptr != cstates.end(), "converter not created yet");
    const auto& converter_state = *converter_state_ptr;
    eosio_assert(converter_state.enabled, "converter is disabled");
    eosio_assert(converter_state.network == from, "converter can only receive from network contract");

    auto contract_name = name(memo_object.path[0].c_str());
    eosio_assert(contract_name == _self, "wrong converter");
    auto base_path_currency = quantity.symbol.code().raw();
    auto middle_path_currency = symbol_code(memo_object.path[1].c_str()).raw();
    auto target_path_currency = symbol_code(memo_object.path[2].c_str()).raw();
    eosio_assert(base_path_currency != target_path_currency, "cannot convert to self");
    auto smart_symbol_name = converter_state.smart_currency.symbol.code().raw();
    eosio_assert(middle_path_currency == smart_symbol_name, "must go through the relay token");
    auto from_connector =  lookup_connector(base_path_currency, converter_state);
    auto to_connector = lookup_connector(target_path_currency, converter_state);

    
    auto base_currency = from_connector.currency;
    auto target_currency = to_connector.currency;
    
    auto target_contract = to_connector.contract;
    auto base_contract = from_connector.contract;

    bool incoming_smart_token = (base_currency.symbol.code().raw() == smart_symbol_name);
    bool outgoing_smart_token = (target_currency.symbol.code().raw() == smart_symbol_name);
    
    auto base_weight = from_connector.weight;
    auto target_weight = to_connector.weight;
    
    eosio_assert(to_connector.enabled, "'to' connector disabled");
    eosio_assert(from_connector.enabled, "'from' connector disabled");
    eosio_assert(code == base_contract, "unknown 'from' contract");
    auto current_base_balance = ((get_balance(base_contract, _self, base_currency.symbol.code())).amount + base_currency.amount - quantity.amount) / pow(10, base_currency.symbol.precision()); 
    auto current_target_balance = ((get_balance(target_contract, _self, target_currency.symbol.code())).amount + target_currency.amount) / pow(10, target_currency.symbol.precision());
    auto current_smart_supply = ((get_supply(converter_state.smart_contract, converter_state.smart_currency.symbol.code())).amount + converter_state.smart_currency.amount)  / pow(10, converter_state.smart_currency.symbol.precision());

    name final_to = name(memo_object.target.c_str());
    real_type smart_tokens = 0;
    real_type target_tokens = 0;
    bool quick = false;
    if (incoming_smart_token) {
        // destory received token
        action(
            permission_level{ _self, "active"_n },
            converter_state.smart_contract, "retire"_n,
            std::make_tuple(quantity, std::string("destroy on conversion"))
        ).send();
        smart_tokens = base_amount;
        current_smart_supply -= smart_tokens;
    }
    else if (!incoming_smart_token && !outgoing_smart_token && (base_weight == target_weight) && (from_connector.fee + to_connector.fee == 0)) {
        target_tokens = quick_convert(current_base_balance, base_amount, current_target_balance);    
        quick = true;
    }
    else {
        smart_tokens = calculate_purchase_return(current_base_balance, base_amount, current_smart_supply, base_weight);
        current_smart_supply += smart_tokens;
        if (from_connector.fee) {
            real_type ffee = (1.0 * from_connector.fee / 1000.0);
            auto fee = smart_tokens * ffee;
            if (from_connector.max_fee > 0 && fee > from_connector.max_fee)
                fee = from_connector.max_fee;
            int64_t feeAmount = (fee * pow(10,converter_state.smart_currency.symbol.precision()));
            if (feeAmount > 0) {
                action(
                    permission_level{_self, "active"_n},
                    converter_state.smart_contract, "issue"_n,
                    std::make_tuple(from_connector.fee_account, asset(feeAmount, converter_state.smart_currency.symbol), std::string("conversion fee"))
                ).send();
                smart_tokens = smart_tokens - fee;
            }
        }
    }

    auto issue = false;
    if (outgoing_smart_token) {
        eosio_assert(memo_object.path.size() == 3, "smarttoken must be final currency");
        target_tokens = smart_tokens;
        issue = true;
    }
    else if (!quick) {
        if (to_connector.fee) {
            real_type ffee = (1.0 * to_connector.fee / 1000.0);
            auto fee = smart_tokens * ffee;
            if (to_connector.max_fee > 0 && fee > to_connector.max_fee)
                fee = to_connector.max_fee;
            int64_t feeAmount = (fee * pow(10, converter_state.smart_currency.symbol.precision()));
            if (feeAmount > 0) {
                action(
                    permission_level{ _self, "active"_n },
                    converter_state.smart_contract, "issue"_n,
                    std::make_tuple(to_connector.fee_account, asset(feeAmount, converter_state.smart_currency.symbol), std::string("conversion fee"))
                ).send();
                smart_tokens = smart_tokens - fee;
            }
        }
        target_tokens = calculate_sale_return(current_target_balance, smart_tokens, current_smart_supply, target_weight);    
    }

    int64_t target_amount = (target_tokens * pow(10, target_currency.symbol.precision()));
    EMIT_CONVERT_EVENT(memo, current_base_balance, current_target_balance, current_smart_supply, base_amount, smart_tokens, target_amount, base_weight, target_weight, to_connector.max_fee, from_connector.max_fee, to_connector.fee, from_connector.fee)
    auto next_hop_memo = next_hop(memo_object);
    auto new_memo = build_memo(next_hop_memo);
    auto new_asset = asset(target_amount,target_currency.symbol);
    name inner_to = converter_state.network;
    if (next_hop_memo.path.size() == 0) {
        inner_to = final_to;
        verify_min_return(new_asset, memo_object.min_return);
        if (converter_state.verify_ram)
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

void BancorConverter::transfer(name from, name to, asset quantity, string memo){
    if (from == _self) {
        // TODO: prevent withdraw of funds.
        return;
    }
    if (to != _self || memo == "initial" || memo == "conversion fee") 
        return;
        
    convert(from , quantity, memo, _code);
}

ACTION BancorConverter::create(name smart_contract,
                               asset smart_currency,
                               bool  smart_enabled,
                               bool  enabled,
                               name  network,
                               bool  verify_ram)
{
    require_auth(_self);
    converters cstates(_self, _self.value);
    auto existing = cstates.find(_self.value);
    if (existing != cstates.end()) {
      cstates.modify(existing, _self, [&](auto& s) {
          s.smart_contract  = smart_contract;
          s.smart_currency  = smart_currency;
          s.smart_enabled   = smart_enabled;
          s.enabled         = enabled;
          s.network         = network;
          s.verify_ram      = verify_ram;
          s.manager         = _self;
      });
    }
    else cstates.emplace(_self, [&](auto& s) {
      s.smart_contract  = smart_contract;
      s.smart_currency  = smart_currency;
      s.smart_enabled   = smart_enabled;
      s.enabled         = enabled;
      s.network         = network;
      s.verify_ram      = verify_ram;
      s.manager         = _self;
    });
}

ACTION BancorConverter::setconnector(name contract,
                                     asset      currency,
                                     uint64_t   weight,
                                     bool       enabled,
                                     uint64_t   fee,
                                     name       fee_account,
                                     uint64_t   max_fee)
{
    require_auth(_self);   
    eosio_assert(fee < 1000, "must be under 1000");
    connectors connectors_table(_self, _self.value);
    auto existing = connectors_table.find(currency.symbol.code().raw());
    if (existing != connectors_table.end()) {
      connectors_table.modify(existing, _self, [&](auto& s) {
          s.contract    = contract;
          s.currency    = currency;
          s.weight      = weight;
          s.enabled     = enabled;
          s.fee         = fee;
          s.fee_account = fee_account;
          s.max_fee     = max_fee;
      });
    }
    else connectors_table.emplace(_self, [&](auto& s) {
        s.contract    = contract;
        s.currency    = currency;
        s.weight      = weight;
        s.enabled     = enabled;
        s.fee         = fee;
        s.fee_account = fee_account;
        s.max_fee     = max_fee;
    });
}

extern "C" {
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        if (action == "transfer"_n.value && code != receiver) {
            eosio::execute_action( eosio::name(receiver), eosio::name(code), &BancorConverter::transfer );
        }
        if (code == receiver){
            switch( action ) { 
                EOSIO_DISPATCH_HELPER( BancorConverter, (create)(setconnector) ) 
            }    
        }
        eosio_exit(0);
    }
}
