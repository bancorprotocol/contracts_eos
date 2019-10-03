#pragma once

#include <eosio/eosio.hpp>
#include <eosio/transaction.hpp>
#include <eosio/asset.hpp>

using namespace eosio;
using namespace std;

/*
    The BancorNetwork contract is the main entry point for bancor token conversions.
    It also allows converting between any token in the bancor network to any other token
    in a single transaction by providing a conversion path.

    A note on conversion path -
    Conversion path is a data structure that's used when converting a token to another token in the bancor network
    when the conversion cannot necessarily be done by single converter and might require multiple 'hops'.
    The path defines which converters should be used and what kind of conversion should be done in each step.

    The path format doesn't include complex structure and instead, it is represented by a space delimited
    list of values in which each 'hop' is represented by a 2-tuple - converter & 'to' token symbol.
    The 'from' token is the token that was sent the the contract with the transfer action.

    Format:
    [converter account, to token symbol, converter account, to token symbol...]

    For example, in order to convert 10 EOS into BNT, the caller needs to transfer 10 EOS to the contract
    and provide the following memo:

    1,bnt2eoscnvrt BNT,1.0000000000,receiver_account_name
*/
CONTRACT BancorNetwork : public eosio::contract {
    public:
        using contract::contract;

        ACTION init();

        // transfer intercepts
        // memo is in csv format, values -
        // version          version number, currently 1
        // path             conversion path, see description above
        // minimum return   conversion minimum return amount, the conversion will fail if the amount returned is lower than the given amount
        // target account   account to receive the conversion return
        void transfer(name from, name to, asset quantity, string memo);
    
    private:
        TABLE settings_t {
            bool enabled;
            uint64_t max_fee;
            
            uint64_t primary_key() const { return "settings"_n.value; }
            
            EOSLIB_SERIALIZE(settings_t, (enabled)(max_fee))
        };

        typedef eosio::multi_index<"settings"_n, settings_t> settings;
        bool isConverter(name converter);
};
