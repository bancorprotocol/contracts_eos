#! /bin/bash
set -x
set -e
./compile.sh

export cleos="docker run --name cleos -v /tmp/work:/tmp/work -v $PWD:/root --rm --link keosd:keosd --link eosio -i eosio/eos-dev:v1.0.4 cleos -u http://jungle.cryptolions.io:18888 --wallet-url http://keosd:8900"
export ROOT_ACCOUNT=$1
export PREFIX_ACCOUNT=jungle.
docker rm -f keosd || true
docker run --name keosd --link eosio:eosio -p 8900:8900 --rm -d eosio/eos-dev:v1.0.4 keosd --http-server-address=0.0.0.0:8900

$cleos wallet create > default.pwd
$cleos wallet import `cat $ROOT_ACCOUNT.private`
function generateUser(){
    $cleos wallet create -n $1 > $1.pwd
    if [ ! -f $PREFIX_ACCOUNT$1.full ]
    then
        $cleos create key > $PREFIX_ACCOUNT$1.full
        cat $PREFIX_ACCOUNT$1.full | grep "Public" | cut -d " " -f 3 > ./$PREFIX_ACCOUNT$1.pub
        cat $PREFIX_ACCOUNT$1.full | grep "Private" | cut -d " " -f 3 > ./$PREFIX_ACCOUNT$1.private
        $cleos wallet import `cat $PREFIX_ACCOUNT$1.private` -n $1
        $cleos system newaccount $ROOT_ACCOUNT $1 `cat $PREFIX_ACCOUNT$1.pub` `cat $PREFIX_ACCOUNT$1.pub` --stake-net "10.0000 EOS" --stake-cpu "10.0000 EOS" --buy-ram "100 EOS"
    else
        $cleos wallet import `cat $PREFIX_ACCOUNT$1.private` -n $1
    fi
}
export -f generateUser
sh setup.sh