#! /bin/bash

set -x
set -e
./compile.sh

export cleos="docker run --name cleos -v /tmp/work:/tmp/work -v $PWD:/root --rm --link keosd:keosd --link eosio:v1.0.8 -i eosio/eos-dev cleos -u http://eosio:8888 --wallet-url http://keosd:8900"

# docker pull eosio/eos-dev
docker rm -f eosio || true
sudo rm -rf /tmp/eosio
docker run --rm --name eosio -d -p 8888:8888 -p 9876:9876 -v /tmp/work:/work -v /tmp/eosio/data:/mnt/dev/data -v /tmp/eosio/config:/mnt/dev/config eosio/eos-dev:v1.0.8  /bin/bash -c "nodeos -e -p eosio --plugin eosio::wallet_api_plugin --plugin eosio::wallet_plugin --plugin eosio::producer_plugin --plugin eosio::history_plugin --plugin eosio::chain_api_plugin --plugin eosio::history_api_plugin --plugin eosio::http_plugin -d /mnt/dev/data --config-dir /mnt/dev/config --http-server-address=0.0.0.0:8888 --access-control-allow-origin=* --contracts-console --max-transaction-time=1000"
sleep 1
docker exec eosio cp -a /contracts /work/contracts

docker rm -f keosd || true
docker run --name keosd --link eosio:eosio -p 8900:8900 --rm -d eosio/eos-dev keosd --http-server-address=0.0.0.0:8900

$cleos wallet create  > default.pwd
$cleos wallet import 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
$cleos set contract eosio /tmp/work/contracts/contracts/eosio.bios -p eosio
export ROOT_ACCOUNT=eosio
function generateUser(){
    $cleos wallet create -n $1 > $1.pwd
    $cleos create key > $1.full
    cat $1.full | grep "Public" | cut -d " " -f 3 > ./$1.pub
    cat $1.full | grep "Private" | cut -d " " -f 3 > ./$1.private
    $cleos wallet import `cat $1.private` -n $1
    $cleos create account $ROOT_ACCOUNT $1 `cat $1.pub` `cat $1.pub`
}
generateUser eosio.token
$cleos set contract eosio.token /tmp/work/contracts/contracts/eosio.token -p eosio.token
$cleos push action eosio.token create "{\"issuer\":\"eosio.token\",\"maximum_supply\":\"1000000000.0000 EOS\"}" -p eosio.token

export -f generateUser
sh setup.sh