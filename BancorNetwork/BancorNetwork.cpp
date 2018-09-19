#include "./BancorNetwork.hpp"
#include "../Common/common.hpp"

using namespace eosio;

void BancorNetwork::on( const currency::transfer& t, account_name code ) {
    if( t.to != _this_contract ) 
        return;
 
    auto a = extended_asset(t.quantity, code);
    eosio_assert( a.is_valid(), "invalid quantity in transfer" );
    eosio_assert( a.amount != 0, "zero quantity is disallowed in transfer");
    auto memoObject = parseMemo(t.memo);
    eosio_assert( memoObject.path.size() >= 3, "bad path format");
    account_name convertContract = string_to_name(memoObject.path[0].c_str());
    
    auto converter_config = converters_table.find(convertContract);
    auto token_config = tokens_table.find(code);
    
    eosio_assert( token_config != tokens_table.end(), "unknown token" );
    eosio_assert( converter_config != converters_table.end(), "unknown converter" );
    
    eosio_assert( token_config->enabled, "token disabled" );
    eosio_assert( converter_config->enabled, "convertor disabled" );
    
    action(
        permission_level{ _self, N(active) },
        code, N(transfer),
        std::make_tuple(_self, convertContract, t.quantity, t.memo)
     ).send();
}

void BancorNetwork::onclear(const clear& act ) {
    require_auth( _self );
    auto it_converter = converters_table.begin();
    while (it_converter != converters_table.end()) {
        it_converter = converters_table.erase(it_converter);
    }
    auto it_token = tokens_table.begin();
    while (it_token != tokens_table.end()) {
        it_token = tokens_table.erase(it_token);
    }
      
}
void BancorNetwork::onregconverter(const regconverter& sc ) {
    require_auth( _self );
    auto existing = converters_table.find(sc.contract);
    if(existing != converters_table.end()){
        converters_table.modify(existing,_self, [&]( auto& s ) {
            s.contract = sc.contract;
            s.enabled = sc.enabled;
        });
    }
    else converters_table.emplace( _self, [&]( auto& s ) {
            s.contract = sc.contract;
            s.enabled = sc.enabled;
         });
}
void BancorNetwork::onregtoken(const regtoken& sc ) {
    require_auth( _self );
    auto existing = tokens_table.find(sc.contract);
    if(existing != tokens_table.end()){
        tokens_table.modify(existing, _self, [&]( auto& s ) {
            s.contract = sc.contract;
            s.enabled = sc.enabled;
        });
    }
    else tokens_table.emplace( _self, [&]( auto& s ) {
        s.contract = sc.contract;
        s.enabled = sc.enabled;
    });
}


void BancorNetwork::apply( account_name contract, account_name act ) {
  if( act == N(transfer) && contract != _self) {
     on( unpack_action_data<currency::transfer>(), contract );
     return;
  }
  if( contract != _this_contract )
     return;
  auto& thiscontract = *this;
  if( name{act} == N(regtoken) ) 
    onregtoken( unpack_action_data<regtoken>() );
  else if( name{act} == N(regconverter) ) 
      onregconverter( unpack_action_data<regconverter>() );
  else if (name{act} == N(clear))
      onclear(unpack_action_data<clear>());
}

extern "C" {
    [[noreturn]] void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        BancorNetwork bn( receiver );
        bn.apply( code, action );
        eosio_exit(0);
    }
}
