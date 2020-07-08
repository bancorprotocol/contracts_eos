#!/bin/bash

# configure
cleos push action bancorcnvrtr setsettings '[{
    "max_fee": 30000,
    "multi_token": "smarttokens1",
    "network": "thisisbancor",
    "staking": ""
}]' -p bancorcnvrtr

# create
cleos push action bancorcnvrtr create '["myaccount", "EOSBNT", 100.0]' -p myaccount

# set reserves
cleos push action bancorcnvrtr setreserve '["EOSBNT", "10,BNT", "bntbntbntbnt", 500000]' -p myaccount
cleos push action bancorcnvrtr setreserve '["EOSBNT", "4,EOS", "eosio.token", 500000]' -p myaccount

# initial funding
cleos transfer myaccount bancorcnvrtr "100.0000000000 BNT" "fund;EOSBNT" --contract bntbntbntbnt
cleos transfer myaccount bancorcnvrtr "100.0000 EOS" "fund;EOSBNT"

# secondary funding
cleos transfer myaccount bancorcnvrtr "500.0000000000 BNT" "fund;EOSBNT" --contract bntbntbntbnt
cleos transfer myaccount bancorcnvrtr "500.0000 EOS" "fund;EOSBNT"
cleos push action bancorcnvrtr fund '["myaccount", "500.0000 EOSBNT"]' -p myaccount

# convert
cleos transfer myaccount thisisbancor "10.0000 EOS" "1,bancorcnvrtr:EOSBNT BNT,0.0,myaccount"
cleos transfer myaccount thisisbancor "9.8360655737 BNT" "1,bancorcnvrtr:EOSBNT EOS,0.0,myaccount" --contract bntbntbntbnt
# //=> should return 10 EOS (9.9999 EOS)