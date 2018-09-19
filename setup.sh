#! /bin/bash
NETWORK=bancorxoneos
NETWORK_TOKEN=BBNT
NETWORK_TOKEN_CONTRACT=bnttoken4eos
TEST_USER=urithelionta
SYSTEM_TOKEN=EOS
SYSTEM_TOKEN_CONTRACT=eosio.token
RLY=RLY
MAIN_RELAY_TOKEN_SYMBOL=$SYSTEM_TOKEN$RLY
MAIN_RELAY=bncrcnvrt111
MAIN_RELAY_TOKEN_CONTRACT=bncrbrly1111

set -x
# set -e

function setPerms(){
    PKEY=$(cat $PREFIX_ACCOUNT$1.pub)
    PERMISSIONS=`echo "{\"threshold\":1,\"keys\":[{\"key\":\"$PKEY\",\"weight\":1}],\"accounts\":[{\"permission\":{\"actor\":\"$1\",\"permission\":\"eosio.code\"},\"weight\":1}]}"`
    $cleos set account permission $1 active $PERMISSIONS owner -p $1
}

function generateToken(){
    generateContract $1 "$2"
    $cleos push action $1 create "$3" -p $1
}

function generateContract(){
    generateUser $1
    $cleos set contract $1 "$2" -p $1 || true
    setPerms $1
}

function generateRelay(){
    RELAY_CONTRACT=$1
    RELAY_TOKEN_CONTRACT=$2
    RELAY_TOKEN_SUPPLY=$3
    RELAY_TOKEN_SYMBOL=$4
    RELAY_TOKEN_RESERVE=$5
    LEFT_TOKEN_CONTRACT=$6
    LEFT_TOKEN_SUPPLY=$7
    LEFT_TOKEN_SYMBOL=$8
    LEFT_TOKEN_RESERVE=$9
    RIGHT_TOKEN_CONTRACT=${10}
    RIGHT_TOKEN_RESERVE=${11}
    RIGHT_TOKEN_SYMBOL=${12}
    FEES=${13}
    generateContract $RELAY_CONTRACT /root/BancorConverter
    generateToken $RELAY_TOKEN_CONTRACT /root/SmartToken "{\"issuer\":\"$RELAY_CONTRACT\",\"maximum_supply\":\"$RELAY_TOKEN_SUPPLY $RELAY_TOKEN_SYMBOL\", \"enabled\": true}"
    generateToken $LEFT_TOKEN_CONTRACT /tmp/work/contracts/contracts/eosio.token "{\"issuer\":\"$LEFT_TOKEN_CONTRACT\",\"maximum_supply\":\"$LEFT_TOKEN_SUPPLY $LEFT_TOKEN_SYMBOL\"}"
    
    $cleos push action $RELAY_TOKEN_CONTRACT issue "[ \"$RELAY_CONTRACT\",\"$RELAY_TOKEN_RESERVE $RELAY_TOKEN_SYMBOL\", \"initial\" ]" -p $RELAY_CONTRACT
    $cleos push action $LEFT_TOKEN_CONTRACT issue "[ \"$RELAY_CONTRACT\",\"$LEFT_TOKEN_RESERVE $LEFT_TOKEN_SYMBOL\", \"initial\" ]" -p $LEFT_TOKEN_CONTRACT
    if [ "$RIGHT_TOKEN_CONTRACT" == "$SYSTEM_TOKEN_CONTRACT" ] && [ "$ROOT_ACCOUNT" != "eosio" ]
    then
        echo "skipping $RIGHT_TOKEN_CONTRACT"
    else
        $cleos push action $RIGHT_TOKEN_CONTRACT issue "[ \"$RELAY_CONTRACT\",\"$RIGHT_TOKEN_RESERVE $RIGHT_TOKEN_SYMBOL\", \"initial\" ]" -p $RIGHT_TOKEN_CONTRACT
    fi

    # register in network    
    $cleos push action $NETWORK regconverter "[\"$RELAY_CONTRACT\", true]" -p $NETWORK

    # register tokens in network
    $cleos push action $NETWORK regtoken "[\"$LEFT_TOKEN_CONTRACT\", true]" -p $NETWORK
    $cleos push action $NETWORK regtoken "[\"$RIGHT_TOKEN_CONTRACT\", true]" -p $NETWORK
    $cleos push action $NETWORK regtoken "[\"$RELAY_TOKEN_CONTRACT\", true]" -p $NETWORK
    
    # convertor
    $cleos push action $RELAY_CONTRACT create "[ \"$RELAY_TOKEN_CONTRACT\", \"0.0000 $RELAY_TOKEN_SYMBOL\", false, true, \"$NETWORK\", false ]" -p $RELAY_CONTRACT
    
    # connectors
    $cleos push action $RELAY_CONTRACT setconnector "[ \"$LEFT_TOKEN_CONTRACT\", \"0.0000 $LEFT_TOKEN_SYMBOL\", 500, true,$FEES,\"$RELAY_CONTRACT\",0 ]" -p $RELAY_CONTRACT
    $cleos push action $RELAY_CONTRACT setconnector "[ \"$RIGHT_TOKEN_CONTRACT\", \"0.0000 $RIGHT_TOKEN_SYMBOL\", 500, true,$FEES,\"$RELAY_CONTRACT\",0 ]" -p $RELAY_CONTRACT
  
}

I=0
function genAdditionalRelay(){
    TOKEN=$3$RLY
    TOKEN=${TOKEN:0:7}
    generateRelay $1 $2 1000000000.0000 $TOKEN 10000.0000 $4 1000000000.0000 $3 1000.0000 $NETWORK_TOKEN_CONTRACT 1000.0000 $NETWORK_TOKEN $5
}

function gen(){
    SUFFIX=`printf "\x$(printf %x $((97+$I)))"`
    genAdditionalRelay bncrcnvrt11$SUFFIX bncrbrly111$SUFFIX $1 bncrbtkn111$SUFFIX $2
    I=$(($I+1))
}

function init(){
    generateContract $NETWORK /root/BancorNetwork
    $cleos push action $NETWORK clear {} -p $NETWORK 
    generateRelay $MAIN_RELAY $MAIN_RELAY_TOKEN_CONTRACT 1000000000.0000 $MAIN_RELAY_TOKEN_SYMBOL 10000.0000 $NETWORK_TOKEN_CONTRACT 1000000000.0000 $NETWORK_TOKEN 1000.0000 $SYSTEM_TOKEN_CONTRACT 1000.0000 $SYSTEM_TOKEN 0
    gen BIQ 0
    gen BATD 0
    gen BHORUS 0
    gen BPRA 0
    gen BONO 0
    gen BYAIR 0
    gen BRIDL 0
    gen BKARMA 0
    gen BEON 0
    gen BBET 0
}



generateUser $TEST_USER

$cleos push action $SYSTEM_TOKEN_CONTRACT transfer "[ \"$TEST_USER\", \"$MAIN_RELAY\", \"100.0000 $SYSTEM_TOKEN\" , \"initial\"]" -p $TEST_USER

if [ "$ROOT_ACCOUNT" == "eosio" ]
then
    $cleos push action $SYSTEM_TOKEN_CONTRACT issue "[ \"$TEST_USER\",\"1000.0000 $SYSTEM_TOKEN\", \"initial\" ]" -p $SYSTEM_TOKEN_CONTRACT
    init
fi
init
$cleos push action $SYSTEM_TOKEN_CONTRACT transfer "[ \"$TEST_USER\", \"$NETWORK\", \"100.0000 $SYSTEM_TOKEN\" , \"1,$MAIN_RELAY $MAIN_RELAY_TOKEN_SYMBOL $NETWORK_TOKEN,0.500,$TEST_USER\"]" -p $TEST_USER

$cleos push action $NETWORK_TOKEN_CONTRACT transfer "[ \"$TEST_USER\", \"$NETWORK\", \"10.0000 $NETWORK_TOKEN\" , \"1,bncrcnvrt11b BATDRLY BATD bncrcnvrt11b BATDRLY BBNT,0.100,$TEST_USER\"]" -p $TEST_USER

$cleos push action $NETWORK_TOKEN_CONTRACT transfer "[ \"$TEST_USER\", \"$NETWORK\", \"10.0000 $NETWORK_TOKEN\" , \"1,bncrcnvrt11a BIQRLY BIQ,0.100,$TEST_USER\"]" -p $TEST_USER
