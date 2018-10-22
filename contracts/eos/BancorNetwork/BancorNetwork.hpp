#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;

using std::string;
using std::vector;

/*
    The BancorNetwork contract is the main entry point for bancor token conversions.
    It also allows converting between any token in the bancor network to any other token
    in a single transaction by providing a conversion path.

    A note on conversion path -
    Conversion path is a data structure that's used when converting a token to another token in the bancor network
    when the conversion cannot necessarily be done by single converter and might require multiple 'hops'.
    The path defines which converters should be used and what kind of conversion should be done in each step.

    The path format doesn't include complex structure and instead, it is represented by a single array
    in which each 'hop' is represented by a 2-tuple - converter & 'to' token symbol.
    The 'from' token is the token that was sent the the contract with the transfer action.

    Format:
    [converter account name, to token symbol, converter account name, to token symbol...]
*/
CONTRACT BancorNetwork : public eosio::contract {
    using contract::contract;
    public:

        ACTION init();

        // transfer intercepts
        // memo is in csv format, values -
        // version          version number, currently 1
        // path             conversion path, see description above
        // minimum return   conversion minimum return amount, the conversion will fail if the amount returned is lower than the given amount
        // target account   account to receive the conversion return
        void transfer(name from, name to, asset quantity, string memo);
};
