#include "./BancorConverter.hpp"
#include "../Common/common.hpp"
#include <math.h>

using namespace eosio;
typedef double real_type;

struct account {
    asset    balance;
    uint64_t primary_key()const { return balance.symbol.code().raw(); }
};

// struct currency_stats {
//     asset         supply;
//     asset         max_supply;
//     name          issuer;
//     uint64_t primary_key()const { return supply.symbol.code().raw(); }
// };

TABLE currency_stats {
    asset        supply;
    asset        max_supply;
    name         issuer;
    bool         enabled;
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

real_type convert_to_exchange(real_type balance, real_type in, real_type supply, int64_t weight) {
    real_type R(supply);
    real_type C(balance+in);
    real_type F(weight/1000.0);
    real_type T(in);
    real_type ONE(1.0);

    real_type E = -R * (ONE - pow(ONE + T / C, F));
    return E;
}

real_type convert_from_exchange(real_type balance, real_type in, real_type supply, int64_t weight) {
    real_type R(supply - in);
    real_type C(balance);
    real_type F(1000.0/weight);
    real_type E(in);
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

void verifyEntry(name account, name currency_contact, eosio::asset currency) {
    accounts accountstable(currency_contact, account.value);
    auto ac = accountstable.find(currency.symbol.code().raw());
    eosio_assert(ac != accountstable.end() , "must have entry for token. (claim token first)");
}

void verifyMinReturn(eosio::asset quantity, std::string min_return) {
	float ret = stof(min_return.c_str());
    int64_t retAmount = (ret * pow(10,quantity.symbol.precision()));
    eosio_assert(quantity.amount >= retAmount, "below min return");
}

const BancorConverter::connector_t& BancorConverter::lookupConnector(uint64_t name, cstate_t state) {
    if (state.smart_currency.symbol.code().raw() == name) {
        static connector_t tempConnector;
        tempConnector.weight = 0;
        tempConnector.fee = 0;
        tempConnector.contract = state.smart_contract;
        tempConnector.currency = state.smart_currency;
        tempConnector.enabled = state.smart_enabled;
        return tempConnector;
    }
    
    connectors connectors_table(_self, _self.value);
    auto existing = connectors_table.find(name);
    eosio_assert(existing != connectors_table.end(), "connector not found");
    return *existing;
}

void BancorConverter::convert(name from, eosio::asset quantity, std::string memo, name code) {
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount != 0, "zero quantity is disallowed");
    auto baseAmount = quantity.amount / pow(10,quantity.symbol.precision());
    auto memoObject = parseMemo(memo);
    eosio_assert(memoObject.path.size() > 2, "invalid memo format");
    converters cstates(_self, _self.value);
    auto converter_state_ptr = cstates.find(_self.value);
    eosio_assert(converter_state_ptr != cstates.end(), "converter not created yet");
    const auto& converter_state = *converter_state_ptr;
    eosio_assert(converter_state.enabled, "converter is disabled");
    eosio_assert(converter_state.network == from, "converter can only receive from network contract");

    auto contractName = name(memoObject.path[0].c_str());
    eosio_assert(contractName == _self, "wrong converter");
    auto basePathCurrency = quantity.symbol.code().raw();
    auto middlePathCurrency = symbol_code(memoObject.path[1].c_str()).raw();
    auto targetPathCurrency = symbol_code(memoObject.path[2].c_str()).raw();
    eosio_assert(basePathCurrency != targetPathCurrency, "cannot convert to self");
    auto smartSymbolName = converter_state.smart_currency.symbol.code().raw();
    eosio_assert(middlePathCurrency == smartSymbolName, "must go through the relay token");
    auto fromConnector =  lookupConnector(basePathCurrency, converter_state);
    auto toConnector = lookupConnector(targetPathCurrency, converter_state);

    
    auto baseCurrency = fromConnector.currency;
    auto targetCurrency = toConnector.currency;
    
    auto targetContract = toConnector.contract;
    auto baseContract = fromConnector.contract;

    bool incomingSmartToken = (baseCurrency.symbol.code().raw() == smartSymbolName);
    bool outgoingSmartToken = (targetCurrency.symbol.code().raw() == smartSymbolName);
    
    auto baseWeight = fromConnector.weight;
    auto targetWeight = toConnector.weight;
    
    eosio_assert(toConnector.enabled, "'to' connector disabled");
    eosio_assert(fromConnector.enabled, "'from' connector disabled");
    eosio_assert(code == baseContract, "unknown 'from' contract");
    auto currentBaseBalance = ((get_balance(baseContract, _self, baseCurrency.symbol.code())).amount + baseCurrency.amount - quantity.amount) / pow(10,baseCurrency.symbol.precision()); 
    auto currentTargetBalance = ((get_balance(targetContract, _self, targetCurrency.symbol.code())).amount + targetCurrency.amount) / pow(10,targetCurrency.symbol.precision());
    auto currentSmartSupply = ((get_supply(converter_state.smart_contract, converter_state.smart_currency.symbol.code())).amount + converter_state.smart_currency.amount)  / pow(10,converter_state.smart_currency.symbol.precision());

    name finalTo = name(memoObject.target.c_str());
    real_type smartTokens = 0;
    real_type targetTokens = 0;
    bool quick = false;
    if (incomingSmartToken) {
        // destory received token
        action(
            permission_level{ _self, "active"_n },
            converter_state.smart_contract, "retire"_n,
            std::make_tuple(quantity,std::string("destroy on conversion"))
        ).send();
        smartTokens = baseAmount;
        currentSmartSupply -= smartTokens;
    }
    else if (!incomingSmartToken && !outgoingSmartToken && (baseWeight == targetWeight) && (fromConnector.fee + toConnector.fee == 0)) {
        targetTokens = quick_convert(currentBaseBalance, baseAmount, currentTargetBalance);    
        quick = true;
    }
    else {
        smartTokens = convert_to_exchange(currentBaseBalance, baseAmount, currentSmartSupply, baseWeight);
        currentSmartSupply += smartTokens;
        if (fromConnector.fee) {
            real_type ffee = (1.0 * fromConnector.fee / 1000.0);
            auto fee = smartTokens * ffee;
            if (fromConnector.maxfee > 0 && fee > fromConnector.maxfee)
                fee = fromConnector.maxfee;
            int64_t feeAmount = (fee * pow(10,converter_state.smart_currency.symbol.precision()));
            if (feeAmount > 0) {
                action(
                    permission_level{_self, "active"_n},
                    converter_state.smart_contract, "issue"_n,
                    std::make_tuple(fromConnector.feeaccount, asset(feeAmount, converter_state.smart_currency.symbol), std::string("conversion fee"))
                ).send();
                smartTokens = smartTokens - fee;
            }
        }
    }

    auto issue = false;
    if (outgoingSmartToken) {
        eosio_assert(memoObject.path.size() == 3, "smarttoken must be final currency");
        targetTokens = smartTokens;
        issue = true;
    }
    else if (!quick) {
        if (toConnector.fee) {
            real_type ffee = (1.0 * toConnector.fee / 1000.0);
            auto fee = smartTokens * ffee;
            if (toConnector.maxfee > 0 && fee > toConnector.maxfee)
                fee = toConnector.maxfee;
            int64_t feeAmount = (fee * pow(10,converter_state.smart_currency.symbol.precision()));
            if (feeAmount > 0) {
                action(
                    permission_level{ _self, "active"_n },
                    converter_state.smart_contract, "issue"_n,
                    std::make_tuple(toConnector.feeaccount, asset(feeAmount, converter_state.smart_currency.symbol), std::string("conversion fee"))
                ).send();
                smartTokens = smartTokens - fee;
            }
        }
        targetTokens = convert_from_exchange(currentTargetBalance, smartTokens, currentSmartSupply, targetWeight);    
    }
    int64_t targetAmount = (targetTokens * pow(10,targetCurrency.symbol.precision()));
    EMIT_CONVERT_EVENT(memo, currentBaseBalance, currentTargetBalance, currentSmartSupply, baseAmount, smartTokens, targetAmount, baseWeight, targetWeight, toConnector.maxfee, fromConnector.maxfee, toConnector.fee, fromConnector.fee)
    auto nextHopMemo = nextHop(memoObject);
    auto newMemo = buildMemo(nextHopMemo);
    auto newAsset = asset(targetAmount,targetCurrency.symbol);
    name inner_to = converter_state.network;
    if (nextHopMemo.path.size() == 0) {
        inner_to = finalTo;
        verifyMinReturn(newAsset, memoObject.min_return);
        if (converter_state.verifyram)
            verifyEntry(inner_to, targetContract ,newAsset);
        newMemo = std::string("convert");
    }
    if (issue)
        action(
            permission_level{ _self, "active"_n },
            targetContract, "issue"_n,
            std::make_tuple(inner_to, newAsset, newMemo) 
        ).send();
    else
        action(
            permission_level{ _self, "active"_n },
            targetContract, "transfer"_n,
            std::make_tuple(_self, inner_to, newAsset, newMemo)
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
                                asset        smart_currency,
                                bool         smart_enabled,
                                bool         enabled,
                                name network,
                                bool         verifyram)
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
          s.verifyram       = verifyram;
          s.manager         = _self;
      });
    }
    else cstates.emplace(_self, [&](auto& s) {
      s.smart_contract  = smart_contract;
      s.smart_currency  = smart_currency;
      s.smart_enabled   = smart_enabled;
      s.enabled         = enabled;
      s.network         = network;
      s.verifyram       = verifyram;
      s.manager         = _self;
    });
}

ACTION BancorConverter::setconnector(name contract,
                    asset        currency,
                    uint64_t     weight,
                    bool         enabled,
                    uint64_t     fee,
                    name feeaccount,
                    uint64_t     maxfee )
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
          s.feeaccount  = feeaccount;
          s.maxfee      = maxfee;
      });
    }
    else connectors_table.emplace(_self, [&](auto& s) {
        s.contract    = contract;
        s.currency    = currency;
        s.weight      = weight;
        s.enabled     = enabled;
        s.fee         = fee;
        s.feeaccount  = feeaccount;
        s.maxfee      = maxfee;
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
