#!/bin/bash
# Usage: ./deploy.sh

# Assumptions: nodeos already running somewhere, 
# wallet created, ENV vars set for wallet name and password
# contracts already compiled (with abi's generated), 
# located in their respective directories, placed into user's root home folder

# Create wallet with command:
#   cleos wallet create -n testwallet --to-console
#   [NOTE] record your password somewhere

# Set ENV vars:
#   export WALLET="testwallet"
#   export PASSWORD="{your password here}"

# In a separate terminal window, run nodeos locally with this exact command:
#   nodeos -e -p eosio --max-transaction-time=1000 --http-validate-host=false --delete-all-blocks --contracts-console --plugin eosio::chain_api_plugin --plugin eosio::history_api_plugin --plugin eosio::producer_plugin --plugin eosio::http_plugin

#=================================================================================#
# Config Constants

CYAN='\033[1;36m'
NC='\033[0m'

# CHANGE PATH
EOSIO_CONTRACTS_ROOT=~/eosio.contracts/build
MY_CONTRACTS_BUILD=~/contracts_eos/build

NODEOS_HOST="127.0.0.1"
NODEOS_PROTOCOL="http"
NODEOS_PORT="8888"
NODEOS_LOCATION="${NODEOS_PROTOCOL}://${NODEOS_HOST}:${NODEOS_PORT}"

alias cleos="cleos --url=${NODEOS_LOCATION}"

BNT_PUB="EOS8HuvjfQeUS7tMdHPPrkTFMnEP7nr6oivvuJyNcvW9Sx5MxJSkZ" 
BNT_PRV="5JS9bTWMc52HWmMC8v58hdfePTxPV5dd5fcxq92xUzbfmafeeRo"

CON_PUB="EOS7pscBeDbJTNn5SNxxowmWwoM7hGj3jDmgxp5KTv7gR89Ny5ii3" 
CON_PRV="5KgKxmnm8oh5WbHC4jmLARNFdkkgVdZ389rdxwGEiBdAJHkubBH"

REP_PUB="EOS833HgCT3egUJRDnW5k3BQGqXAEDmoYo6q1s7wWnovn6B9Mb1pd" 
REP_PRV="5JFLPVygcZZdEno2WWWkf3fPriuxnvjtVpkThifYM5HwcKg6ndu"

USR_PUB="EOS8UAsFY4RacdaeuadicrkP66JQxPsbNyucmbT8Z4GjwFoytsK9u" 
USR_PRV="5JKAjH9WH4XnZCEe8v5Wir7awV4YBTVa8KUSqWJbQR6QGtj4yce"


if [[ -z "${WALLET}" ]]; then
  echo "ERROR: run 'export WALLET=[wallet_name] to set your wallet name'"
  exit 1
else
  WALLET="${WALLET}"
fi

if [[ -z "${PASSWORD}" ]]; then
  echo "ERROR: run 'export PASSWORD=[password] to set your wallet password'"
  exit 1
else
  PASSWORD="${PASSWORD}"
fi


# 0) Unlock wallet

echo "=== UNLOCKING WALLET ==="
cleos wallet unlock -n $WALLET --password $PASSWORD

# EOSIO system-related keys
echo -e "${CYAN}-----------------------SYSTEM KEYS-----------------------${NC}"
cleos wallet import -n $WALLET --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
cleos wallet import -n $WALLET --private-key 5JgqWJYVBcRhviWZB3TU1tN9ui6bGpQgrXVtYZtTG2d3yXrDtYX
cleos wallet import -n $WALLET --private-key 5JjjgrrdwijEUU2iifKF94yKduoqfAij4SKk6X5Q3HfgHMS4Ur6
cleos wallet import -n $WALLET --private-key 5HxJN9otYmhgCKEbsii5NWhKzVj2fFXu3kzLhuS75upN5isPWNL
cleos wallet import -n $WALLET --private-key 5JNHjmgWoHiG9YuvX2qvdnmToD2UcuqavjRW5Q6uHTDtp3KG3DS
cleos wallet import -n $WALLET --private-key 5JZkaop6wjGe9YY8cbGwitSuZt8CjRmGUeNMPHuxEDpYoVAjCFZ
cleos wallet import -n $WALLET --private-key 5Hroi8WiRg3by7ap3cmnTpUoqbAbHgz3hGnGQNBYFChswPRUt26
cleos wallet import -n $WALLET --private-key 5JbMN6pH5LLRT16HBKDhtFeKZqe7BEtLBpbBk5D7xSZZqngrV8o
cleos wallet import -n $WALLET --private-key 5JUoVWoLLV3Sj7jUKmfE8Qdt7Eo7dUd4PGZ2snZ81xqgnZzGKdC
cleos wallet import -n $WALLET --private-key 5Ju1ree2memrtnq8bdbhNwuowehZwZvEujVUxDhBqmyTYRvctaF

# Create system accounts
echo -e "${CYAN}-----------------------SYSTEM ACCOUNTS-----------------------${NC}"
cleos create account eosio eosio.bpay EOS7gFoz5EB6tM2HxdV9oBjHowtFipigMVtrSZxrJV3X6Ph4jdPg3
cleos create account eosio eosio.msig EOS6QRncHGrDCPKRzPYSiWZaAw7QchdKCMLWgyjLd1s2v8tiYmb45
cleos create account eosio eosio.names EOS7ygRX6zD1sx8c55WxiQZLfoitYk2u8aHrzUxu6vfWn9a51iDJt
cleos create account eosio eosio.ram EOS5tY6zv1vXoqF36gUg5CG7GxWbajnwPtimTnq6h5iptPXwVhnLC
cleos create account eosio eosio.ramfee EOS6a7idZWj1h4PezYks61sf1RJjQJzrc8s4aUbe3YJ3xkdiXKBhF
cleos create account eosio eosio.saving EOS8ioLmKrCyy5VyZqMNdimSpPjVF2tKbT5WKhE67vbVPcsRXtj5z
cleos create account eosio eosio.stake EOS5an8bvYFHZBmiCAzAtVSiEiixbJhLY8Uy5Z7cpf3S9UoqA3bJb
cleos create account eosio eosio.token EOS7JPVyejkbQHzE9Z4HwewNzGss11GB21NPkwTX2MQFmruYFqGXm
cleos create account eosio eosio.vpay EOS6szGbnziz224T1JGoUUFu2LynVG72f8D3UVAS25QgwawdH983U

# Bootstrap new system contracts
echo -e "${CYAN}-----------------------SYSTEM CONTRACTS-----------------------${NC}"
cleos set contract eosio.token $EOSIO_CONTRACTS_ROOT/eosio.token/
cleos set contract eosio.msig $EOSIO_CONTRACTS_ROOT/eosio.msig/
cleos push action eosio.token create '[ "eosio", "100000000000.0000 EOS" ]' -p eosio.token
cleos push action eosio.token create '[ "eosio", "100000000000.0000 SYS" ]' -p eosio.token
echo -e "      EOS TOKEN CREATED"
cleos push action eosio.token issue '[ "eosio", "10000000000.0000 EOS", "memo" ]' -p eosio
cleos push action eosio.token issue '[ "eosio", "10000000000.0000 SYS", "memo" ]' -p eosio
echo -e "      EOS TOKEN ISSUED"
cleos set contract eosio $EOSIO_CONTRACTS_ROOT/eosio.bios/
echo -e "      BIOS SET"
cleos set contract eosio $EOSIO_CONTRACTS_ROOT/eosio.system/
echo -e "      SYSTEM SET"
cleos push action eosio setpriv '["eosio.msig", 1]' -p eosio@active
cleos push action eosio init '[0, "4,EOS"]' -p eosio@active
#cleos push action eosio init '[0, "4,SYS"]' -p eosio@active

# Deploy eosio.wrap
echo -e "${CYAN}-----------------------EOSIO WRAP-----------------------${NC}"
cleos wallet import -n $WALLET --private-key 5J3JRDhf4JNhzzjEZAsQEgtVuqvsPPdZv4Tm6SjMRx1ZqToaray
cleos system newaccount eosio eosio.wrap EOS7LpGN1Qz5AbCJmsHzhG7sWEGd9mwhTXWmrYXqxhTknY2fvHQ1A --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos push action eosio setpriv '["eosio.wrap", 1]' -p eosio@active
cleos set contract eosio.wrap $EOSIO_CONTRACTS_ROOT/eosio.wrap/

# 1) Import user keys

echo -e "${CYAN}-----------------------CONTRACT / USER KEYS-----------------------${NC}"
cleos wallet import -n $WALLET --private-key $REP_PRV
cleos wallet import -n $WALLET --private-key $BNT_PRV
cleos wallet import -n $WALLET --private-key $CON_PRV
cleos wallet import -n $WALLET --private-key $USR_PRV

# 2) Create accounts

cleos system newaccount eosio reporter1 $REP_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio reporter2 $REP_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio reporter3 $REP_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio reporter4 $REP_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer

cleos system newaccount eosio bancorxoneos $BNT_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio thisisbancor $BNT_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio bntbntbntbnt $BNT_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio bntxrerouter $BNT_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer

cleos system newaccount eosio stakebnt2eos $CON_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio bnt2eoscnvrt $CON_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio bnt2eosrelay $CON_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer

cleos system newaccount eosio bnttestuser1 $USR_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio bnttestuser2 $USR_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio fakeos $USR_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer

cleos system newaccount eosio bnt2syscnvrt $USR_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos system newaccount eosio bnt2sysrelay $USR_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer

# 3) Deploy contracts

echo -e "${CYAN}-----------------------DEPLOYING CONTRACTS-----------------------${NC}"
cleos set contract fakeos $EOSIO_CONTRACTS_ROOT/eosio.token/

cleos set contract bntxrerouter $MY_CONTRACTS_BUILD XTransferRerouter.wasm XTransferRerouter.abi
cleos set contract thisisbancor $MY_CONTRACTS_BUILD BancorNetwork.wasm BancorNetwork.abi
cleos set contract bancorxoneos $MY_CONTRACTS_BUILD BancorX.wasm BancorX.abi
cleos set contract bntbntbntbnt $MY_CONTRACTS_BUILD Token.wasm Token.abi

cleos set contract bnt2eoscnvrt $MY_CONTRACTS_BUILD BancorConverter.wasm BancorConverter.abi
cleos set contract bnt2syscnvrt $MY_CONTRACTS_BUILD BancorConverter.wasm BancorConverter.abi
cleos set contract stakebnt2eos $MY_CONTRACTS_BUILD relayswarmio.wasm relayswarmio.abi
cleos set contract bnt2eosrelay $MY_CONTRACTS_BUILD Token.wasm Token.abi
cleos set contract bnt2sysrelay $MY_CONTRACTS_BUILD Token.wasm Token.abi

# 4) Set Permissions

cleos set account permission bntxrerouter active '{ "threshold": 1, "keys": [{ "key": "EOS8HuvjfQeUS7tMdHPPrkTFMnEP7nr6oivvuJyNcvW9Sx5MxJSkZ", "weight": 1 }], "accounts": [{ "permission": { "actor":"bntxrerouter","permission":"eosio.code" }, "weight":1 }] }' owner -p bntxrerouter 
cleos set account permission thisisbancor active '{ "threshold": 1, "keys": [{ "key": "EOS8HuvjfQeUS7tMdHPPrkTFMnEP7nr6oivvuJyNcvW9Sx5MxJSkZ", "weight": 1 }], "accounts": [{ "permission": { "actor":"thisisbancor","permission":"eosio.code" }, "weight":1 }] }' owner -p thisisbancor
cleos set account permission bancorxoneos active '{ "threshold": 1, "keys": [{ "key": "EOS8HuvjfQeUS7tMdHPPrkTFMnEP7nr6oivvuJyNcvW9Sx5MxJSkZ", "weight": 1 }], "accounts": [{ "permission": { "actor":"bancorxoneos","permission":"eosio.code" }, "weight":1 }] }' owner -p bancorxoneos
cleos set account permission bntbntbntbnt active '{ "threshold": 1, "keys": [{ "key": "EOS8HuvjfQeUS7tMdHPPrkTFMnEP7nr6oivvuJyNcvW9Sx5MxJSkZ", "weight": 1 }], "accounts": [{ "permission": { "actor":"bntbntbntbnt","permission":"eosio.code" }, "weight":1 }] }' owner -p bntbntbntbnt

cleos set account permission bnt2eoscnvrt active '{ "threshold": 1, "keys": [{ "key": "EOS7pscBeDbJTNn5SNxxowmWwoM7hGj3jDmgxp5KTv7gR89Ny5ii3", "weight": 1 }], "accounts": [{ "permission": { "actor":"bnt2eoscnvrt","permission":"eosio.code" }, "weight":1 }] }' owner -p bnt2eoscnvrt
cleos set account permission bnt2eosrelay active '{ "threshold": 1, "keys": [{ "key": "EOS7pscBeDbJTNn5SNxxowmWwoM7hGj3jDmgxp5KTv7gR89Ny5ii3", "weight": 1 }], "accounts": [{ "permission": { "actor":"bnt2eosrelay","permission":"eosio.code" }, "weight":1 }] }' owner -p bnt2eosrelay
cleos set account permission stakebnt2eos active '{ "threshold": 1, "keys": [{ "key": "EOS7pscBeDbJTNn5SNxxowmWwoM7hGj3jDmgxp5KTv7gR89Ny5ii3", "weight": 1 }], "accounts": [{ "permission": { "actor":"stakebnt2eos","permission":"eosio.code" }, "weight":1 }] }' owner -p stakebnt2eos

cleos set account permission bnt2syscnvrt active '{ "threshold": 1, "keys": [{ "key": "EOS8UAsFY4RacdaeuadicrkP66JQxPsbNyucmbT8Z4GjwFoytsK9u", "weight": 1 }], "accounts": [{ "permission": { "actor":"bnt2syscnvrt","permission":"eosio.code" }, "weight":1 }] }' owner -p bnt2syscnvrt
cleos set account permission bnt2sysrelay active '{ "threshold": 1, "keys": [{ "key": "EOS8UAsFY4RacdaeuadicrkP66JQxPsbNyucmbT8Z4GjwFoytsK9u", "weight": 1 }], "accounts": [{ "permission": { "actor":"bnt2sysrelay","permission":"eosio.code" }, "weight":1 }] }' owner -p bnt2sysrelay
