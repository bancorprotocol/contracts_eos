#!/bin/bash

# unlock wallet
cleos wallet unlock --password $(cat ~/eosio-wallet/.pass)

# create accounts
cleos create account eosio bancorcnvrtr EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio myaccount EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio eosio.token EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio tethertether EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio smarttokens1 EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio bntbntbntbnt EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
cleos create account eosio thisisbancor EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV

# deploy
cleos set contract bancorcnvrtr . BancorConverter.wasm BancorConverter.abi
cleos set contract thisisbancor ../BancorNetwork BancorNetwork.wasm BancorNetwork.abi
cleos set contract eosio.token  ../Token Token.wasm Token.abi
cleos set contract tethertether ../Token Token.wasm Token.abi
cleos set contract bntbntbntbnt ../Token Token.wasm Token.abi
cleos set contract smarttokens1 ../Token Token.wasm Token.abi

# permission
cleos set account permission bancorcnvrtr active --add-code
cleos set account permission smarttokens1 active bancorcnvrtr --add-code
cleos set account permission thisisbancor active --add-code

# create BNT token
cleos push action bntbntbntbnt create '["bntbntbntbnt", "100000000.0000000000 BNT"]' -p bntbntbntbnt
cleos push action bntbntbntbnt issue '["bntbntbntbnt", "5000000.0000000000 BNT", "init"]' -p bntbntbntbnt
cleos transfer bntbntbntbnt myaccount "50000.0000000000 BNT" "init" --contract bntbntbntbnt

# create USDT token
cleos push action tethertether create '["tethertether", "100000000.0000 USDT"]' -p tethertether
cleos push action tethertether issue '["tethertether", "5000000.0000 USDT", "init"]' -p tethertether
cleos transfer tethertether myaccount "50000.0000 USDT" "init" --contract tethertether

# create EOS token
cleos push action eosio.token create '["eosio", "100000000.0000 EOS"]' -p eosio.token
cleos push action eosio.token issue '["eosio", "5000000.0000 EOS", "init"]' -p eosio
cleos transfer eosio myaccount "50000.0000 EOS" "init"

# setup Bancor Network
cleos push action thisisbancor setmaxfee '[30000]' -p thisisbancor
cleos push action thisisbancor setnettoken '["bntbntbntbnt"]' -p thisisbancor
