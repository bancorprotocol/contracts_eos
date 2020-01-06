#!/bin/bash
source ./scripts/deploy/default.conf

# test keys
MASTER_PUB_KEY="EOS8HuvjfQeUS7tMdHPPrkTFMnEP7nr6oivvuJyNcvW9Sx5MxJSkZ" 
MASTER_PRV_KEY="5JS9bTWMc52HWmMC8v58hdfePTxPV5dd5fcxq92xUzbfmafeeRo"


CYAN='\033[1;36m'
GREEN='\033[0;32m'
NC='\033[0m'

EOSIO_CONTRACTS_ROOT=./contracts/eos/eosio.contracts/build
MY_CONTRACTS_BUILD=./contracts/eos

NODEOS_HOST="127.0.0.1"
NODEOS_PROTOCOL="http"
NODEOS_PORT="8888"
# NODEOS_LOCATION="${NODEOS_PROTOCOL}://${NODEOS_HOST}:${NODEOS_PORT}"

while getopts ":u:" opt; do
  case $opt in
    u)
      NODEOS_LOCATION=$OPTARG
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      exit 1
      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      exit 1
      ;;
  esac
done

# temp keosd setup
WALLET_DIR=/tmp/temp-eosio-wallet
UNIX_SOCKET_ADDRESS=$WALLET_DIR/keosd.sock
WALLET_URL=unix://$UNIX_SOCKET_ADDRESS

function cleos() { command cleos --verbose --url=${NODEOS_LOCATION} --wallet-url=$WALLET_URL "$@"; echo $@; }
function ensureaccount() {
  local account_name=${1}
  local pub_key=${2}

  cleos system newaccount eosio "$account_name" "$pub_key" --stake-cpu "50 EOS" --stake-net "10 EOS" --buy-ram-kbytes 5000 --transfer
}

ensureaccount bntreporter1 $MASTER_PUB_KEY
ensureaccount bntreporter2 $MASTER_PUB_KEY
ensureaccount bntreporter3 $MASTER_PUB_KEY

ensureaccount bancorxoneos $MASTER_PUB_KEY
ensureaccount thisisbancor $MASTER_PUB_KEY
ensureaccount bntbntbntbnt $MASTER_PUB_KEY
ensureaccount bntxrerouter $MASTER_PUB_KEY

ensureaccount bnt2eoscnvrt $MASTER_PUB_KEY
ensureaccount bnt2eosrelay $MASTER_PUB_KEY

ensureaccount multiconvert $MASTER_PUB_KEY
ensureaccount multi4tokens $MASTER_PUB_KEY
ensureaccount multistaking $MASTER_PUB_KEY


# 3) Deploy contracts

echo -e "${CYAN}-----------------------DEPLOYING CONTRACTS-----------------------${NC}"

cleos set contract bntxrerouter $MY_CONTRACTS_BUILD/XTransferRerouter

cleos set contract bntxrerouter $MY_CONTRACTS_BUILD/XTransferRerouter
cleos set contract thisisbancor $MY_CONTRACTS_BUILD/BancorNetwork
cleos set contract bancorxoneos $MY_CONTRACTS_BUILD/BancorX
cleos set contract bntbntbntbnt $MY_CONTRACTS_BUILD/Token

cleos set contract bnt2eoscnvrt $MY_CONTRACTS_BUILD/BancorConverter
cleos set contract bnt2eosrelay $EOSIO_CONTRACTS_ROOT/eosio.token/ 
cleos set contract multiconvert $MY_CONTRACTS_BUILD/MultiConverter
cleos set contract multi4tokens $EOSIO_CONTRACTS_ROOT/eosio.token/



# 4) Set Permissions

cleos set account permission bntxrerouter active '{ "threshold": 1, "keys": [{ "key": "EOS8HuvjfQeUS7tMdHPPrkTFMnEP7nr6oivvuJyNcvW9Sx5MxJSkZ", "weight": 1 }], "accounts": [{ "permission": { "actor":"bntxrerouter","permission":"eosio.code" }, "weight":1 }] }' owner -p bntxrerouter 
cleos set account permission thisisbancor active '{ "threshold": 1, "keys": [{ "key": "EOS8HuvjfQeUS7tMdHPPrkTFMnEP7nr6oivvuJyNcvW9Sx5MxJSkZ", "weight": 1 }], "accounts": [{ "permission": { "actor":"thisisbancor","permission":"eosio.code" }, "weight":1 }] }' owner -p thisisbancor
cleos set account permission bancorxoneos active '{ "threshold": 1, "keys": [{ "key": "EOS8HuvjfQeUS7tMdHPPrkTFMnEP7nr6oivvuJyNcvW9Sx5MxJSkZ", "weight": 1 }], "accounts": [{ "permission": { "actor":"bancorxoneos","permission":"eosio.code" }, "weight":1 }] }' owner -p bancorxoneos
cleos set account permission bntbntbntbnt active '{ "threshold": 1, "keys": [{ "key": "EOS8HuvjfQeUS7tMdHPPrkTFMnEP7nr6oivvuJyNcvW9Sx5MxJSkZ", "weight": 1 }], "accounts": [{ "permission": { "actor":"bntbntbntbnt","permission":"eosio.code" }, "weight":1 }] }' owner -p bntbntbntbnt

cleos set account permission bnt2eoscnvrt active '{ "threshold": 1, "keys": [{ "key": "'$MASTER_PUB_KEY'", "weight": 1 }], "accounts": [{ "permission": { "actor":"bnt2eoscnvrt","permission":"eosio.code" }, "weight":1 }] }' owner -p bnt2eoscnvrt
cleos set account permission bnt2eosrelay active '{ "threshold": 1, "keys": [{ "key": "'$MASTER_PUB_KEY'", "weight": 1 }], "accounts": [{ "permission": { "actor":"bnt2eosrelay","permission":"eosio.code" }, "weight":1 }] }' owner -p bnt2eosrelay

cleos set account permission multiconvert active '{ "threshold": 1, "keys": [{ "key": "'$MASTER_PUB_KEY'", "weight": 1 }], "accounts": [{ "permission": { "actor":"multiconvert","permission":"eosio.code" }, "weight":1 }] }' owner -p multiconvert
cleos set account permission multi4tokens active '{ "threshold": 1, "keys": [{ "key": "'$MASTER_PUB_KEY'", "weight": 1 }], "accounts": [{ "permission": { "actor":"multi4tokens","permission":"eosio.code" }, "weight":1 }] }' owner -p multi4tokens
cleos set account permission multi4tokens active multiconvert --add-code



# Contracts Initialization

if [ $INIT_BANCOR_X == true ] ; then # BancorX
  cleos push action bancorxoneos init '["bntbntbntbnt", "2", "1", "100000000000000", "10000000000000000", "10000000000000000"]' -p bancorxoneos
  cleos push action bancorxoneos enablerpt '["1"]' -p bancorxoneos
  cleos push action bancorxoneos enablext '["1"]' -p bancorxoneos
  cleos push action bancorxoneos addreporter '["bntreporter1"]' -p bancorxoneos
  cleos push action bancorxoneos addreporter '["bntreporter2"]' -p bancorxoneos
  cleos push action bancorxoneos addreporter '["bntreporter3"]' -p bancorxoneos
fi

if [ $INIT_MULTI_CONVERTER == true ] ; then # MultiConverter
  cleos push action multiconvert setmultitokn '["multi4tokens"]' -p multiconvert
  cleos push action multiconvert setstaking '["multistaking"]' -p multiconvert
  cleos push action multiconvert setmaxfee '["30000"]' -p multiconvert
fi

if [ $INIT_BNT == true ] ; then # BNT Token & Converter
  cleos push action bnt2eoscnvrt init '["bnt2eosrelay", "0.00000000 BNTEOS", "1", "thisisbancor", require_balance: false, max_fee: 30000, fee: fee]' -p bnt2eoscnvrt
fi

if [ $INIT_BANCOR_NETWORK == true ] ; then # BancorNetwork
  cleos push action thisisbancor setmaxfee '["30000"]' -p thisisbancor
fi
