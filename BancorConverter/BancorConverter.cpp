#include "./BancorConverter.hpp"
#include "../Common/common.hpp"
#include <math.h>
using namespace eosio;
using namespace eosiosystem;

struct currency_stats {
    asset          supply;
    asset          max_supply;
    account_name   issuer;
    bool           enabled;
    uint64_t primary_key()const { return supply.symbol.name(); }
};
typedef eosio::multi_index<N(stat), currency_stats> stats;
struct account {
    asset    balance;
    uint64_t primary_key()const { return balance.symbol.name(); }
};
typedef eosio::multi_index<N(accounts), account> accounts;

asset get_balance( account_name contract, account_name owner, symbol_name sym )
{
  accounts accountstable( contract, owner );
  const auto& ac = accountstable.get( sym );
  return ac.balance;
}      

asset get_supply( account_name contract, symbol_name sym )
{
  stats statstable( contract, sym );
  const auto& st = statstable.get( sym );
  return st.supply;
}

real_type convert_to_exchange(real_type balance, real_type in, real_type supply, int64_t weight ) {
  real_type R(supply);
  real_type C(balance+in);
  real_type F(weight/1000.0);
  real_type T(in);
  real_type ONE(1.0);

  real_type E = -R * (ONE - pow( ONE + T / C, F) );
  return E;

}

real_type convert_from_exchange( real_type balance, real_type in, real_type supply, int64_t weight) {
  real_type R(supply - in);
  real_type C(balance);
  real_type F(1000.0/weight);
  real_type E(in);
  real_type ONE(1.0);

  real_type T = C * (pow( ONE + E/R, F) - ONE);
  return T;
}


real_type quick_convert(real_type balance, real_type in, real_type targetBalance){
  return in / (balance + in) * targetBalance;
}
   
float stof(const char* s){
  float rez = 0, fact = 1;
  if (*s == '-'){
    s++;
    fact = -1;
  };
  for (int point_seen = 0; *s; s++){
    if (*s == '.'){
      point_seen = 1; 
      continue;
    };
    int d = *s - '0';
    if (d >= 0 && d <= 9){
      if (point_seen) fact /= 10.0f;
      rez = rez * 10.0f + (float)d;
    };
  };
  return rez * fact;
};
void verifyEntry(account_name account, account_name currency_contact, eosio::asset currency){
    accounts accountstable( currency_contact, account );
    auto ac = accountstable.find( currency.symbol.name() );
    eosio_assert(ac != accountstable.end() , "must have entry for token. (claim token first)");
}
void verifyMinReturn(eosio::asset quantity, std::string min_return){
	float ret = stof(min_return.c_str());
    int64_t retAmount = (ret * pow(10,quantity.symbol.precision()));
    eosio_assert( quantity.amount >= retAmount, "below min return");
}

const BancorConverter::connector& BancorConverter::lookupConnector(uint64_t name, cstate state ){
    if(state.smart_currency.symbol.name() == name){
        static connector tempConnector;
        tempConnector.weight = 0;
        tempConnector.fee = 0;
        tempConnector.contract = state.smart_contract;
        tempConnector.currency = state.smart_currency;
        tempConnector.enabled = state.smart_enabled;
        return tempConnector;
    }
    auto existing = connectors_table.find( name );
    eosio_assert( existing != connectors_table.end(), "connector not found" );
    return *existing;
}
void BancorConverter::convert( account_name from, eosio::asset quantity, std::string memo, account_name code) {
    auto a = extended_asset(quantity, code);
    eosio_assert( a.is_valid(), "invalid quantity" );
    eosio_assert( a.amount != 0, "zero quantity is disallowed");
    auto baseAmount = quantity.amount / pow(10,quantity.symbol.precision());
    auto memoObject = parseMemo(memo);
    eosio_assert(memoObject.path.size() > 2, "invalid memo format" );
    auto converter_state_ptr = cstates.find( _self );
    eosio_assert( converter_state_ptr != cstates.end(), "converter not created yet" );
    const auto& converter_state = *converter_state_ptr;
    eosio_assert( converter_state.enabled, "converter is disabled" );
    eosio_assert( converter_state.network == from, "converter can only receive from network contract" );

    auto contractName = string_to_name(memoObject.path[0].c_str());
    eosio_assert( contractName == _self, "wrong converter" );
    
    auto basePathCurrency = quantity.symbol.name();
    auto middlePathCurrency = symbol_type(string_to_symbol(3,memoObject.path[1].c_str())).name();
    auto targetPathCurrency = symbol_type(string_to_symbol(3,memoObject.path[2].c_str())).name();
    eosio_assert(basePathCurrency != targetPathCurrency, "cannot convert to self" );
    auto smartSymbolName = converter_state.smart_currency.symbol.name();
    eosio_assert(middlePathCurrency == smartSymbolName, "must go through the relay token" );
    auto fromConnector =  lookupConnector( basePathCurrency, converter_state);
    auto toConnector = lookupConnector( targetPathCurrency, converter_state);

    
    auto baseCurrency = fromConnector.currency;
    auto targetCurrency = toConnector.currency;
    
    auto targetContract = toConnector.contract;
    auto baseContract = fromConnector.contract;

    bool incomingSmartToken = (baseCurrency.symbol.name() == smartSymbolName);
    bool outgoingSmartToken = (targetCurrency.symbol.name() == smartSymbolName);
    
    auto baseWeight = fromConnector.weight;
    auto targetWeight = toConnector.weight;
    
    eosio_assert( toConnector.enabled, "'to' connector disabled" );
    eosio_assert( fromConnector.enabled, "'from' connector disabled" );
    eosio_assert( code == baseContract, "unknown 'from' contract" );
    auto currentBaseBalance = ((get_balance(baseContract, _self, baseCurrency.symbol.name())).amount + baseCurrency.amount) / pow(10,baseCurrency.symbol.precision()); 
    auto currentTargetBalance = ((get_balance(targetContract, _self, targetCurrency.symbol.name())).amount + targetCurrency.amount) / pow(10,targetCurrency.symbol.precision());
    auto currentSmartSupply = ((get_supply(converter_state.smart_contract, converter_state.smart_currency.symbol.name())).amount + converter_state.smart_currency.amount)  / pow(10,converter_state.smart_currency.symbol.precision());
    
    account_name finalTo = string_to_name(memoObject.target.c_str());
    real_type smartTokens = 0;
    real_type targetTokens = 0;
    bool quick = false;
    if(incomingSmartToken){
        // destory received token
        action(
            permission_level{ _self, N(active) },
            converter_state.smart_contract, N(retire),
            std::make_tuple(quantity,std::string("destroy on conversion"))
        ).send();
        smartTokens = baseAmount;
    }
    else if(!incomingSmartToken && !outgoingSmartToken && (baseWeight == targetWeight) && (fromConnector.fee + toConnector.fee == 0)){
        targetTokens = quick_convert(currentBaseBalance, baseAmount, currentTargetBalance);    
        quick = true;
    }
    else{
        smartTokens = convert_to_exchange(currentBaseBalance, baseAmount, currentSmartSupply, baseWeight);
        if(fromConnector.fee){
            real_type ffee = (1.0 * fromConnector.fee / 1000.0);
            auto fee = smartTokens * ffee;
            if(fromConnector.maxfee > 0 && fee > fromConnector.maxfee)
                fee = fromConnector.maxfee;
            int64_t feeAmount = (fee * pow(10,converter_state.smart_currency.symbol.precision()));
            if(feeAmount > 0){
                action(
                    permission_level{ _self, N(active) },
                    converter_state.smart_contract, N(issue),
                    std::make_tuple( fromConnector.feeaccount, asset(feeAmount, converter_state.smart_currency.symbol), std::string("conversion fee"))
                ).send();
                smartTokens = smartTokens - fee;
            }
        }
    }
    auto issue = false;
    if(outgoingSmartToken){
        eosio_assert(memoObject.path.size() == 3, "smarttoken must be final currency" );
        targetTokens = smartTokens;
        issue = true;
    }
    else if(!quick){
        if(toConnector.fee){
            real_type ffee = (1.0 * toConnector.fee / 1000.0);
            auto fee = smartTokens * ffee;
            if(toConnector.maxfee > 0 && fee > toConnector.maxfee)
                fee = toConnector.maxfee;
            int64_t feeAmount = (fee * pow(10,converter_state.smart_currency.symbol.precision()));
            if(feeAmount > 0){
                action(
                    permission_level{ _self, N(active) },
                    converter_state.smart_contract, N(issue),
                    std::make_tuple( toConnector.feeaccount, asset(feeAmount, converter_state.smart_currency.symbol), std::string("conversion fee"))
                ).send();
                smartTokens = smartTokens - fee;
            }
        }
        targetTokens = convert_from_exchange(currentTargetBalance, smartTokens, currentSmartSupply, targetWeight );    
    }
    int64_t targetAmount = (targetTokens * pow(10,targetCurrency.symbol.precision()));
    print("stats = " ,currentBaseBalance, " " , currentTargetBalance, " " ,currentSmartSupply," " ,baseAmount, " " , smartTokens, " ", targetAmount," ",  baseWeight," " , targetWeight," ",toConnector.maxfee, " ",fromConnector.maxfee, " ",toConnector.fee, " ",fromConnector.fee, " \n");
    auto nextHopMemo = nextHop(memoObject);
    auto newMemo = buildMemo(nextHopMemo);
    auto newAsset = asset(targetAmount,targetCurrency.symbol);
    account_name inner_to = converter_state.network;
    if(nextHopMemo.path.size() == 0){
        inner_to = finalTo;
        verifyMinReturn(newAsset, memoObject.min_return);
        if(converter_state.verifyram)
            verifyEntry(inner_to, targetContract ,newAsset);
        newMemo = std::string("convert");
    }
    if(issue)
        action(
            permission_level{ _self, N(active) },
            targetContract, N(issue),
            std::make_tuple( inner_to, newAsset, newMemo) 
        ).send();
    else
        action(
            permission_level{ _self, N(active) },
            targetContract, N(transfer),
            std::make_tuple(_self, inner_to, newAsset, newMemo)
        ).send();
}

void BancorConverter::on( const currency::transfer& t, account_name code ) {
    if( t.from == _self ){
        // TODO: prevent withdraw of funds.
        return;
    }
    if( t.to != _self || t.memo == "initial" || t.memo == "conversion fee") 
        return;
    convert( t.from , t.quantity, t.memo, code);
}

void BancorConverter::oncreate(const create& sc, account_name code ) {
    require_auth( _self );
    auto existing = cstates.find( _self );
    if(existing != cstates.end()){
      cstates.modify(existing, _self, [&]( auto& s ) {
          s.smart_contract  = sc.smart_contract;
          s.smart_currency  = sc.smart_currency;
          s.smart_enabled   = sc.smart_enabled;
          s.enabled         = sc.enabled;
          s.network         = sc.network;
          s.verifyram       = sc.verifyram;
          s.manager         = _self;
      });
    }
    else cstates.emplace( _self, [&]( auto& s ) {
      s.smart_contract  = sc.smart_contract;
      s.smart_currency  = sc.smart_currency;
      s.smart_enabled   = sc.smart_enabled;
      s.enabled         = sc.enabled;
      s.network         = sc.network;
      s.verifyram       = sc.verifyram;
      s.manager         = _self;
    });
}


void BancorConverter::onsetconnector(const BancorConverter::setconnector& sc, account_name code ){
    require_auth( _self );   
    eosio_assert( sc.fee < 1000, "must be under 1000" );
    auto existing = connectors_table.find( sc.currency.symbol.name() );
    if(existing != connectors_table.end()){
      connectors_table.modify(existing, _self, [&]( auto& s ) {
          s.contract    = sc.contract;
          s.currency    = sc.currency;
          s.weight      = sc.weight;
          s.enabled     = sc.enabled;
          s.fee         = sc.fee;
          s.feeaccount  = sc.feeaccount;
          s.maxfee      = sc.maxfee;
      });
    }
    else connectors_table.emplace( _self, [&]( auto& s ) {
        s.contract    = sc.contract;
        s.currency    = sc.currency;
        s.weight      = sc.weight;
        s.enabled     = sc.enabled;
        s.fee         = sc.fee;
        s.feeaccount  = sc.feeaccount;
        s.maxfee      = sc.maxfee;
    });
}

void BancorConverter::apply( account_name contract, account_name act ) {
  if( act == N(transfer)) {
     on( unpack_action_data<currency::transfer>(), contract );
     return;
  }
  if( contract != _self )
     return;
  auto& thiscontract = *this;
  if( name{act} == N(create) ) {
      oncreate( unpack_action_data<create>(), contract );
  }
  else if( name{act} == N(setconnector) ) {
      onsetconnector( unpack_action_data<setconnector>(), contract );
  };
}

extern "C" {
    [[noreturn]] void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        BancorConverter bc( receiver );
        bc.apply( code, action );
        eosio_exit(0);
    }
}