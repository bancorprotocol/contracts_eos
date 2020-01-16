#!/bin/bash
source ./scripts/deploy/config/common.conf
eval "$(jq -r 'to_entries | .[] | .key + "=\"" + .value + "\""' < ./config/contract_names.json)"

MODE="local"
while getopts ":u:w:m:" opt; do
  case $opt in
    u)
      NODEOS_ENDPOINT=$OPTARG
      ;;
    w)
      WALLET_URL=$OPTARG
      ;;
    m)
      MODE=$OPTARG
      ;;
    *)
      echo "Option -$OPTARG requires an argument." >&2
      exit 1
      ;;
  esac
done

if [ $MODE == "remote" ] ; then
  read -p "Enter nodeos endpoint: " NODEOS_ENDPOINT
  if [[ $NODEOS_ENDPOINT == *"localhost"* ]] || [[ $NODEOS_ENDPOINT == *"127.0.0.1"* ]] ; then
    echo "NODEOS_ENDPOINT can't be a local endpoint. (received: ${NODEOS_ENDPOINT})"
    exit 1
  fi
  read -p "Enter master account name: " MASTER_ACCOUNT
  ACCOUNT_BALANCE=$(cleos get table eosio.token $MASTER_ACCOUNT accounts | jq .rows[0].balance | cut -d'.' -f 1 | cut -c2-)
  # if (($ACCOUNT_BALANCE<=1000)) ; then
  #   echo "MASTER_ACCOUNT must have more than 1000 EOS (but it only got $ACCOUNT_BALANCE EOS)"
  #   exit 1
  # fi
  read -p "Enter master public key " MASTER_PUB_KEY
  read -p "Enter master private key: " MASTER_PRV_KEY
elif [ $MODE != "local" ] ; then
  echo "Mode flag (-m) must be either 'local' or 'remote' (received: ${MODE})"
  exit 1
fi

function cleos() { command cleos --verbose --url=${NODEOS_ENDPOINT} --wallet-url=${WALLET_URL} "$@"; }
on_exit(){
  echo -e "${CYAN}cleaning up temporary keosd process & artifacts${NC}";
  kill -9 $TEMP_KEOSD_PID &> /dev/null
  rm -r $WALLET_DIR
}
trap on_exit INT
trap on_exit SIGINT

# start temp keosd
rm -rf $WALLET_DIR
mkdir $WALLET_DIR
keosd --wallet-dir=$WALLET_DIR --unix-socket-path=$UNIX_SOCKET_ADDRESS &> /dev/null &
TEMP_KEOSD_PID=$!
sleep 1

# create temp wallet
cleos wallet create --file /tmp/.temp_keosd_pwd
PASSWORD=$(cat /tmp/.temp_keosd_pwd)
cleos wallet unlock --password="$PASSWORD"

cleos wallet import --private-key $MASTER_PRV_KEY

function ensureaccount() {
  local account_name=${1}
  local pub_key=${2}

  cleos get account "$account_name" &>/dev/null
  if [ "$?" != 0 ]
  then
    cleos system newaccount $MASTER_ACCOUNT "$account_name" "$pub_key" --stake-cpu "10 EOS" --stake-net "5 EOS" --buy-ram-kbytes 5000 --transfer
  fi
}

ensureaccount $REPORTER_1_ACCOUNT $MASTER_PUB_KEY
ensureaccount $REPORTER_2_ACCOUNT $MASTER_PUB_KEY
ensureaccount $REPORTER_3_ACCOUNT $MASTER_PUB_KEY

ensureaccount $BANCOR_X_ACCOUNT $MASTER_PUB_KEY
ensureaccount $BANCOR_NETWORK_ACCOUNT $MASTER_PUB_KEY
ensureaccount $BNT_TOKEN_ACCOUNT $MASTER_PUB_KEY
ensureaccount $TX_REROUTER_ACCOUNT $MASTER_PUB_KEY

ensureaccount $MULTI_CONVERTER_ACCOUNT $MASTER_PUB_KEY
ensureaccount $MULTI_TOKEN_ACCOUNT $MASTER_PUB_KEY
ensureaccount $MULTI_STAKING_ACCOUNT $MASTER_PUB_KEY


# 3) Deploy contracts

echo -e "${CYAN}-----------------------DEPLOYING CONTRACTS-----------------------${NC}"

cleos set contract $TX_REROUTER_ACCOUNT $MY_CONTRACTS_BUILD/XTransferRerouter
cleos set contract $BANCOR_NETWORK_ACCOUNT $MY_CONTRACTS_BUILD/BancorNetwork
cleos set contract $BANCOR_X_ACCOUNT $MY_CONTRACTS_BUILD/BancorX
cleos set contract $BNT_TOKEN_ACCOUNT $MY_CONTRACTS_BUILD/Token

cleos set contract $MULTI_CONVERTER_ACCOUNT $MY_CONTRACTS_BUILD/MultiConverter
cleos set contract $MULTI_TOKEN_ACCOUNT $EOSIO_CONTRACTS_ROOT/eosio.token/


# 4) Set Permissions

cleos set account permission $TX_REROUTER_ACCOUNT active '{ "threshold": 1, "keys": [{ "key": "'$MASTER_PUB_KEY'", "weight": 1 }], "accounts": [{ "permission": { "actor":"'$TX_REROUTER_ACCOUNT'","permission":"eosio.code" }, "weight":1 }] }' owner -p $TX_REROUTER_ACCOUNT 
cleos set account permission $BANCOR_NETWORK_ACCOUNT active '{ "threshold": 1, "keys": [{ "key": "'$MASTER_PUB_KEY'", "weight": 1 }], "accounts": [{ "permission": { "actor":"'$BANCOR_NETWORK_ACCOUNT'","permission":"eosio.code" }, "weight":1 }] }' owner -p $BANCOR_NETWORK_ACCOUNT
cleos set account permission $BANCOR_X_ACCOUNT active '{ "threshold": 1, "keys": [{ "key": "'$MASTER_PUB_KEY'", "weight": 1 }], "accounts": [{ "permission": { "actor":"'$BANCOR_X_ACCOUNT'","permission":"eosio.code" }, "weight":1 }] }' owner -p $BANCOR_X_ACCOUNT
cleos set account permission $BNT_TOKEN_ACCOUNT active '{ "threshold": 1, "keys": [{ "key": "'$MASTER_PUB_KEY'", "weight": 1 }], "accounts": [{ "permission": { "actor":"'$BNT_TOKEN_ACCOUNT'","permission":"eosio.code" }, "weight":1 }] }' owner -p $BNT_TOKEN_ACCOUNT

cleos set account permission $MULTI_CONVERTER_ACCOUNT active '{ "threshold": 1, "keys": [{ "key": "'$MASTER_PUB_KEY'", "weight": 1 }], "accounts": [{ "permission": { "actor":"'$MULTI_CONVERTER_ACCOUNT'","permission":"eosio.code" }, "weight":1 }] }' owner -p $MULTI_CONVERTER_ACCOUNT
cleos set account permission $MULTI_TOKEN_ACCOUNT active '{ "threshold": 1, "keys": [{ "key": "'$MASTER_PUB_KEY'", "weight": 1 }], "accounts": [{ "permission": { "actor":"'$MULTI_TOKEN_ACCOUNT'","permission":"eosio.code" }, "weight":1 }] }' owner -p $MULTI_TOKEN_ACCOUNT
cleos set account permission $MULTI_TOKEN_ACCOUNT active $MULTI_CONVERTER_ACCOUNT --add-code



# Contracts Initialization

ROWS=$(cleos get table $BANCOR_X_ACCOUNT $BANCOR_X_ACCOUNT settings | jq .rows | jq length)
if (($ROWS==0)) ; then # BancorX
  cleos push action $BANCOR_X_ACCOUNT init '["'$BNT_TOKEN_ACCOUNT'", "2", "1", "100000000000000", "10000000000000000", "10000000000000000"]' -p $BANCOR_X_ACCOUNT
  cleos push action $BANCOR_X_ACCOUNT enablerpt '["1"]' -p $BANCOR_X_ACCOUNT
  cleos push action $BANCOR_X_ACCOUNT enablext '["1"]' -p $BANCOR_X_ACCOUNT
  cleos push action $BANCOR_X_ACCOUNT addreporter '["'$REPORTER_1_ACCOUNT'"]' -p $BANCOR_X_ACCOUNT
  cleos push action $BANCOR_X_ACCOUNT addreporter '["'$REPORTER_2_ACCOUNT'"]' -p $BANCOR_X_ACCOUNT
  cleos push action $BANCOR_X_ACCOUNT addreporter '["'$REPORTER_3_ACCOUNT'"]' -p $BANCOR_X_ACCOUNT
fi

ROWS=$(cleos get table $MULTI_CONVERTER_ACCOUNT $MULTI_CONVERTER_ACCOUNT settings | jq .rows | jq length)
if (($ROWS==0)) ; then # MultiConverter
  # if [[ $MODE == "remote" ]] ; then
    cleos push action $MULTI_CONVERTER_ACCOUNT setmultitokn '["'$MULTI_TOKEN_ACCOUNT'"]' -p $MULTI_CONVERTER_ACCOUNT
    cleos push action $MULTI_CONVERTER_ACCOUNT setstaking '["'$MULTI_STAKING_ACCOUNT'"]' -p $MULTI_CONVERTER_ACCOUNT
    cleos push action $MULTI_CONVERTER_ACCOUNT setmaxfee '["30000"]' -p $MULTI_CONVERTER_ACCOUNT  
  # fi
  cleos push action $MULTI_CONVERTER_ACCOUNT setnetwork '["'$BANCOR_NETWORK_ACCOUNT'"]' -p $MULTI_CONVERTER_ACCOUNT
fi

ROWS=$(cleos get table $BNT_TOKEN_ACCOUNT BNT stat | jq .rows | jq length)
if (($ROWS==0)) ; then # BNT Token & Converter
  # if [[ $MODE == "remote" ]] ; then
  cleos push action $BNT_TOKEN_ACCOUNT create '["'$BANCOR_X_ACCOUNT'", "250000000.00000000 BNT"]' -p $BNT_TOKEN_ACCOUNT
  # fi
  cleos push action $MULTI_CONVERTER_ACCOUNT create '["'$MASTER_ACCOUNT'", "BNTEOS", "9900.00000000"]' -p $MASTER_ACCOUNT

  cleos push action $MULTI_CONVERTER_ACCOUNT setreserve '["BNTEOS", "8,BNT", "'$BNT_TOKEN_ACCOUNT'", "500000"]' -p $MASTER_ACCOUNT
  cleos push action $MULTI_CONVERTER_ACCOUNT setreserve '["BNTEOS", "4,EOS", "eosio.token", "500000"]' -p $MASTER_ACCOUNT
  
  cleos push action $BNT_TOKEN_ACCOUNT issue '[ "'$BANCOR_X_ACCOUNT'", "10000.00000000 BNT", ""]' -p $BANCOR_X_ACCOUNT
  cleos push action $BNT_TOKEN_ACCOUNT transfer '["'$BANCOR_X_ACCOUNT'", '"$MASTER_ACCOUNT"', "10000.00000000 BNT", ""]' -p $BANCOR_X_ACCOUNT

  cleos push action $BNT_TOKEN_ACCOUNT transfer '["'$MASTER_ACCOUNT'", "'$MULTI_CONVERTER_ACCOUNT'", "99.00000000 BNT", "fund;BNTEOS"]' -p $MASTER_ACCOUNT
  cleos push action eosio.token transfer '["'$MASTER_ACCOUNT'", "'$MULTI_CONVERTER_ACCOUNT'", "99.0000 EOS", "fund;BNTEOS"]' -p $MASTER_ACCOUNT

  cleos push action $BNT_TOKEN_ACCOUNT transfer '["'$MASTER_ACCOUNT'", "'$TEST_ACCOUNT'", "3000.00000000 BNT", ""]' -p $MASTER_ACCOUNT
fi

ROWS=$(cleos get table $BANCOR_NETWORK_ACCOUNT $BANCOR_NETWORK_ACCOUNT settings | jq .rows | jq length)
if (($ROWS==0)) ; then # BancorNetwork
  cleos push action $BANCOR_NETWORK_ACCOUNT setmaxfee '["30000"]' -p $BANCOR_NETWORK_ACCOUNT
  cleos push action $BANCOR_NETWORK_ACCOUNT setnettoken '["'$BNT_TOKEN_ACCOUNT'"]' -p $BANCOR_NETWORK_ACCOUNT
fi

on_exit
echo -e "${GREEN}--> Done${NC}"
