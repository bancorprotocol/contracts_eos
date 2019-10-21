#!/bin/bash
# Usage: ./deploy.sh

# Assumptions: contracts already compiled (with abi's generated);
# in a separate terminal window, nodeos is running locally with this exact command:
#   nodeos -e -p eosio --max-transaction-time=1000 --http-validate-host=false --delete-all-blocks --contracts-console --plugin eosio::chain_api_plugin --plugin eosio::history_api_plugin --plugin eosio::producer_plugin --plugin eosio::http_plugin

#=================================================================================#
# Config Constants

CYAN='\033[1;36m'
GREEN='\033[0;32m'
NC='\033[0m'

EOSIO_CONTRACTS_ROOT=./contracts/eos/eosio.contracts/build
MY_CONTRACTS_BUILD=./contracts/eos

NODEOS_HOST="127.0.0.1"
NODEOS_PROTOCOL="http"
NODEOS_PORT="8888"
NODEOS_LOCATION="${NODEOS_PROTOCOL}://${NODEOS_HOST}:${NODEOS_PORT}"

# temp keosd setup
WALLET_DIR=/tmp/temp-eosio-wallet
UNIX_SOCKET_ADDRESS=$WALLET_DIR/keosd.sock
WALLET_URL=unix://$UNIX_SOCKET_ADDRESS

function cleos() { command cleos --verbose --url=${NODEOS_LOCATION} --wallet-url=$WALLET_URL "$@"; echo $@; }
on_exit(){
  echo -e "${CYAN}cleaning up temporary keosd process & artifacts${NC}";
  kill -9 $TEMP_KEOSD_PID &> /dev/null
  rm -r $WALLET_DIR
}
trap my_trap INT
trap my_trap SIGINT


# start temp keosd
rm -r $WALLET_DIR
mkdir $WALLET_DIR
keosd --wallet-dir=$WALLET_DIR --unix-socket-path=$UNIX_SOCKET_ADDRESS &> /dev/null &
TEMP_KEOSD_PID=$!
sleep 1

# create temp wallet
cleos wallet create --file /tmp/.temp_keosd_pwd
PASSWORD=$(cat /tmp/.temp_keosd_pwd)
cleos wallet unlock --password="$PASSWORD"


# test keys
BNT_PUB="EOS8HuvjfQeUS7tMdHPPrkTFMnEP7nr6oivvuJyNcvW9Sx5MxJSkZ" 
BNT_PRV="5JS9bTWMc52HWmMC8v58hdfePTxPV5dd5fcxq92xUzbfmafeeRo"

CON_PUB="EOS7pscBeDbJTNn5SNxxowmWwoM7hGj3jDmgxp5KTv7gR89Ny5ii3" 
CON_PRV="5KgKxmnm8oh5WbHC4jmLARNFdkkgVdZ389rdxwGEiBdAJHkubBH"

REP_PUB="EOS833HgCT3egUJRDnW5k3BQGqXAEDmoYo6q1s7wWnovn6B9Mb1pd" 
REP_PRV="5JFLPVygcZZdEno2WWWkf3fPriuxnvjtVpkThifYM5HwcKg6ndu"

USR_PUB="EOS8UAsFY4RacdaeuadicrkP66JQxPsbNyucmbT8Z4GjwFoytsK9u" 
USR_PRV="5JKAjH9WH4XnZCEe8v5Wir7awV4YBTVa8KUSqWJbQR6QGtj4yce"


# EOSIO system-related keys
echo -e "${CYAN}-----------------------SYSTEM KEYS-----------------------${NC}"
cleos wallet import --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
cleos wallet import --private-key 5JgqWJYVBcRhviWZB3TU1tN9ui6bGpQgrXVtYZtTG2d3yXrDtYX
cleos wallet import --private-key 5JjjgrrdwijEUU2iifKF94yKduoqfAij4SKk6X5Q3HfgHMS4Ur6
cleos wallet import --private-key 5HxJN9otYmhgCKEbsii5NWhKzVj2fFXu3kzLhuS75upN5isPWNL
cleos wallet import --private-key 5JNHjmgWoHiG9YuvX2qvdnmToD2UcuqavjRW5Q6uHTDtp3KG3DS
cleos wallet import --private-key 5JZkaop6wjGe9YY8cbGwitSuZt8CjRmGUeNMPHuxEDpYoVAjCFZ
cleos wallet import --private-key 5Hroi8WiRg3by7ap3cmnTpUoqbAbHgz3hGnGQNBYFChswPRUt26
cleos wallet import --private-key 5JbMN6pH5LLRT16HBKDhtFeKZqe7BEtLBpbBk5D7xSZZqngrV8o
cleos wallet import --private-key 5JUoVWoLLV3Sj7jUKmfE8Qdt7Eo7dUd4PGZ2snZ81xqgnZzGKdC
cleos wallet import --private-key 5Ju1ree2memrtnq8bdbhNwuowehZwZvEujVUxDhBqmyTYRvctaF


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
cleos create account eosio eosio.rex EOS6szGbnziz224T1JGoUUFu2LynVG72f8D3UVAS25QgwawdH983U

# Activate consensus upgrade
echo -e "${CYAN}-----------------------ACTIVATING BIOS, PROTOCOL FEATURES-----------------------${NC}"
cleos set contract eosio $EOSIO_CONTRACTS_ROOT/eosio.bios/
echo -e "      BIOS SET"
cleos push action eosio activate '["f0af56d2c5a48d60a4a5b5c903edfb7db3a736a94ed589d0b797df33ff9d3e1d"]' -p eosio # GET_SENDER
cleos push action eosio activate '["2652f5f96006294109b3dd0bbde63693f55324af452b799ee137a81a905eed25"]' -p eosio # FORWARD_SETCODE
cleos push action eosio activate '["8ba52fe7a3956c5cd3a656a3174b931d3bb2abb45578befc59f283ecd816a405"]' -p eosio # ONLY_BILL_FIRST_AUTHORIZER
cleos push action eosio activate '["ad9e3d8f650687709fd68f4b90b41f7d825a365b02c23a636cef88ac2ac00c43"]' -p eosio # RESTRICT_ACTION_TO_SELF
cleos push action eosio activate '["68dcaa34c0517d19666e6b33add67351d8c5f69e999ca1e37931bc410a297428"]' -p eosio # DISALLOW_EMPTY_PRODUCER_SCHEDULE
cleos push action eosio activate '["e0fb64b1085cc5538970158d05a009c24e276fb94e1a0bf6a528b48fbc4ff526"]' -p eosio # FIX_LINKAUTH_RESTRICTION
cleos push action eosio activate '["ef43112c6543b88db2283a2e077278c315ae2c84719a8b25f25cc88565fbea99"]' -p eosio # REPLACE_DEFERRED
cleos push action eosio activate '["4a90c00d55454dc5b059055ca213579c6ea856967712a56017487886a4d4cc0f"]' -p eosio # NO_DUPLICATE_DEFERRED_ID
cleos push action eosio activate '["1a99a59d87e06e09ec5b028a9cbb7749b4a5ad8819004365d02dc4379a8b7241"]' -p eosio # ONLY_LINK_TO_EXISTING_PERMISSION
cleos push action eosio activate '["4e7bf348da00a945489b2a681749eb56f5de00b900014e137ddae39f48f69d67"]' -p eosio # RAM_RESTRICTIONS

# Bootstrap new system contracts
echo -e "${CYAN}-----------------------SYSTEM CONTRACTS-----------------------${NC}"
cleos set contract eosio.token $EOSIO_CONTRACTS_ROOT/eosio.token/ 
cleos set contract eosio.msig $EOSIO_CONTRACTS_ROOT/eosio.msig/ 
cleos push action eosio setpriv '["eosio.msig", 1]' -p eosio

cleos push action eosio.token create '[ "eosio", "100000000000.0000 EOS" ]' -p eosio.token
cleos push action eosio.token create '[ "eosio", "100000000000.0000 SYS" ]' -p eosio.token
echo -e "      EOS TOKEN CREATED"
cleos push action eosio.token issue '[ "eosio", "10000000000.0000 EOS", "memo" ]' -p eosio
cleos push action eosio.token issue '[ "eosio", "10000000000.0000 SYS", "memo" ]' -p eosio
echo -e "      EOS TOKEN ISSUED"

cleos set contract eosio $EOSIO_CONTRACTS_ROOT/eosio.system/
echo -e "      SYSTEM SET"
cleos push action eosio init '[0, "4,EOS"]' -p eosio

# Deploy eosio.wrap
echo -e "${CYAN}-----------------------EOSIO WRAP-----------------------${NC}"
cleos wallet import --private-key 5J3JRDhf4JNhzzjEZAsQEgtVuqvsPPdZv4Tm6SjMRx1ZqToaray
cleos system newaccount eosio eosio.wrap EOS7LpGN1Qz5AbCJmsHzhG7sWEGd9mwhTXWmrYXqxhTknY2fvHQ1A --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos push action eosio setpriv '["eosio.wrap", 1]' -p eosio
cleos set contract eosio.wrap $EOSIO_CONTRACTS_ROOT/eosio.wrap/


# 1) Import user keys

echo -e "${CYAN}-----------------------CONTRACT / USER KEYS-----------------------${NC}"
cleos wallet import --private-key "$REP_PRV"
cleos wallet import --private-key "$BNT_PRV"
cleos wallet import --private-key "$CON_PRV"
cleos wallet import --private-key "$USR_PRV"


# 2) Create accounts

cleos system newaccount eosio reporter1 $REP_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio reporter2 $REP_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio reporter3 $REP_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio reporter4 $REP_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer

cleos system newaccount eosio bancorxoneos $BNT_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio thisisbancor $BNT_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio bntbntbntbnt $BNT_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio bntxrerouter $BNT_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer

cleos system newaccount eosio bnt2eoscnvrt $CON_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio bnt2eosrelay $CON_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer

cleos system newaccount eosio bnttestuser1 $USR_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio bnttestuser2 $USR_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio fakeos $USR_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer

cleos system newaccount eosio bnt2syscnvrt $USR_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos system newaccount eosio bnt2sysrelay $USR_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer

cleos system newaccount eosio bnt2aaacnvrt $USR_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos system newaccount eosio bnt2aaarelay $USR_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos system newaccount eosio aaa $USR_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer

cleos system newaccount eosio bnt2bbbcnvrt $USR_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos system newaccount eosio bnt2bbbrelay $USR_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer
cleos system newaccount eosio bbb $USR_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 50000 --transfer

cleos system newaccount eosio multiconvert $CON_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio multi4tokens $CON_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
cleos system newaccount eosio multistaking $CON_PUB --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer


# 3) Deploy contracts

echo -e "${CYAN}-----------------------DEPLOYING CONTRACTS-----------------------${NC}"
cleos set contract fakeos $EOSIO_CONTRACTS_ROOT/eosio.token/
cleos set contract aaa $EOSIO_CONTRACTS_ROOT/eosio.token/
cleos set contract bbb $EOSIO_CONTRACTS_ROOT/eosio.token/

cleos set contract bntxrerouter $MY_CONTRACTS_BUILD/XTransferRerouter
cleos set contract thisisbancor $MY_CONTRACTS_BUILD/BancorNetwork
cleos set contract bancorxoneos $MY_CONTRACTS_BUILD/BancorX
cleos set contract bntbntbntbnt $MY_CONTRACTS_BUILD/Token

cleos set contract bnt2eoscnvrt $MY_CONTRACTS_BUILD/BancorConverter
cleos set contract bnt2syscnvrt $MY_CONTRACTS_BUILD/BancorConverter
cleos set contract bnt2aaacnvrt $MY_CONTRACTS_BUILD/BancorConverter
cleos set contract bnt2bbbcnvrt $MY_CONTRACTS_BUILD/BancorConverter

cleos set contract bnt2eosrelay $EOSIO_CONTRACTS_ROOT/eosio.token/ 
cleos set contract bnt2sysrelay $EOSIO_CONTRACTS_ROOT/eosio.token/ 
cleos set contract bnt2aaarelay $EOSIO_CONTRACTS_ROOT/eosio.token/ 
cleos set contract bnt2bbbrelay $EOSIO_CONTRACTS_ROOT/eosio.token/

cleos set contract multiconvert $MY_CONTRACTS_BUILD/MultiConverter
cleos set contract multi4tokens $EOSIO_CONTRACTS_ROOT/eosio.token/ 


# 4) Set Permissions

cleos set account permission bntxrerouter active '{ "threshold": 1, "keys": [{ "key": "EOS8HuvjfQeUS7tMdHPPrkTFMnEP7nr6oivvuJyNcvW9Sx5MxJSkZ", "weight": 1 }], "accounts": [{ "permission": { "actor":"bntxrerouter","permission":"eosio.code" }, "weight":1 }] }' owner -p bntxrerouter 
cleos set account permission thisisbancor active '{ "threshold": 1, "keys": [{ "key": "EOS8HuvjfQeUS7tMdHPPrkTFMnEP7nr6oivvuJyNcvW9Sx5MxJSkZ", "weight": 1 }], "accounts": [{ "permission": { "actor":"thisisbancor","permission":"eosio.code" }, "weight":1 }] }' owner -p thisisbancor
cleos set account permission bancorxoneos active '{ "threshold": 1, "keys": [{ "key": "EOS8HuvjfQeUS7tMdHPPrkTFMnEP7nr6oivvuJyNcvW9Sx5MxJSkZ", "weight": 1 }], "accounts": [{ "permission": { "actor":"bancorxoneos","permission":"eosio.code" }, "weight":1 }] }' owner -p bancorxoneos
cleos set account permission bntbntbntbnt active '{ "threshold": 1, "keys": [{ "key": "EOS8HuvjfQeUS7tMdHPPrkTFMnEP7nr6oivvuJyNcvW9Sx5MxJSkZ", "weight": 1 }], "accounts": [{ "permission": { "actor":"bntbntbntbnt","permission":"eosio.code" }, "weight":1 }] }' owner -p bntbntbntbnt

cleos set account permission bnt2eoscnvrt active '{ "threshold": 1, "keys": [{ "key": "EOS7pscBeDbJTNn5SNxxowmWwoM7hGj3jDmgxp5KTv7gR89Ny5ii3", "weight": 1 }], "accounts": [{ "permission": { "actor":"bnt2eoscnvrt","permission":"eosio.code" }, "weight":1 }] }' owner -p bnt2eoscnvrt
cleos set account permission bnt2eosrelay active '{ "threshold": 1, "keys": [{ "key": "EOS7pscBeDbJTNn5SNxxowmWwoM7hGj3jDmgxp5KTv7gR89Ny5ii3", "weight": 1 }], "accounts": [{ "permission": { "actor":"bnt2eosrelay","permission":"eosio.code" }, "weight":1 }] }' owner -p bnt2eosrelay

cleos set account permission bnt2syscnvrt active '{ "threshold": 1, "keys": [{ "key": "EOS8UAsFY4RacdaeuadicrkP66JQxPsbNyucmbT8Z4GjwFoytsK9u", "weight": 1 }], "accounts": [{ "permission": { "actor":"bnt2syscnvrt","permission":"eosio.code" }, "weight":1 }] }' owner -p bnt2syscnvrt
cleos set account permission bnt2sysrelay active '{ "threshold": 1, "keys": [{ "key": "EOS8UAsFY4RacdaeuadicrkP66JQxPsbNyucmbT8Z4GjwFoytsK9u", "weight": 1 }], "accounts": [{ "permission": { "actor":"bnt2sysrelay","permission":"eosio.code" }, "weight":1 }] }' owner -p bnt2sysrelay

cleos set account permission bnt2aaacnvrt active '{ "threshold": 1, "keys": [{ "key": "EOS8UAsFY4RacdaeuadicrkP66JQxPsbNyucmbT8Z4GjwFoytsK9u", "weight": 1 }], "accounts": [{ "permission": { "actor":"bnt2aaacnvrt","permission":"eosio.code" }, "weight":1 }] }' owner -p bnt2aaacnvrt
cleos set account permission bnt2aaarelay active '{ "threshold": 1, "keys": [{ "key": "EOS8UAsFY4RacdaeuadicrkP66JQxPsbNyucmbT8Z4GjwFoytsK9u", "weight": 1 }], "accounts": [{ "permission": { "actor":"bnt2aaarelay","permission":"eosio.code" }, "weight":1 }] }' owner -p bnt2aaarelay

cleos set account permission bnt2bbbcnvrt active '{ "threshold": 1, "keys": [{ "key": "EOS8UAsFY4RacdaeuadicrkP66JQxPsbNyucmbT8Z4GjwFoytsK9u", "weight": 1 }], "accounts": [{ "permission": { "actor":"bnt2bbbcnvrt","permission":"eosio.code" }, "weight":1 }] }' owner -p bnt2bbbcnvrt
cleos set account permission bnt2bbbrelay active '{ "threshold": 1, "keys": [{ "key": "EOS8UAsFY4RacdaeuadicrkP66JQxPsbNyucmbT8Z4GjwFoytsK9u", "weight": 1 }], "accounts": [{ "permission": { "actor":"bnt2bbbrelay","permission":"eosio.code" }, "weight":1 }] }' owner -p bnt2bbbrelay

cleos set account permission multiconvert active '{ "threshold": 1, "keys": [{ "key": "EOS7pscBeDbJTNn5SNxxowmWwoM7hGj3jDmgxp5KTv7gR89Ny5ii3", "weight": 1 }], "accounts": [{ "permission": { "actor":"multiconvert","permission":"eosio.code" }, "weight":1 }] }' owner -p multiconvert
cleos set account permission multi4tokens active '{ "threshold": 1, "keys": [{ "key": "EOS7pscBeDbJTNn5SNxxowmWwoM7hGj3jDmgxp5KTv7gR89Ny5ii3", "weight": 1 }], "accounts": [{ "permission": { "actor":"multi4tokens","permission":"eosio.code" }, "weight":1 }] }' owner -p multi4tokens
cleos set account permission multi4tokens active multiconvert --add-code


on_exit
echo -e "${GREEN}--> Done${NC}"
