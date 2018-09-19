#! /bin/bash
set -x
set -e
eosiocpp="docker run --name cpp -v $PWD:/root --rm -i eosio/eos-dev:v1.0.8 eosiocpp"


$eosiocpp -g /root/BancorNetwork/BancorNetwork.abi /root/BancorNetwork/BancorNetwork.cpp
$eosiocpp -o /root/BancorNetwork/BancorNetwork.wasm /root/BancorNetwork/BancorNetwork.cpp

$eosiocpp -g /root/BancorConverter/BancorConverter.abi /root/BancorConverter/BancorConverter.cpp
$eosiocpp -o /root/BancorConverter/BancorConverter.wasm /root/BancorConverter/BancorConverter.cpp


$eosiocpp -g /root/SmartToken/SmartToken.abi /root/SmartToken/SmartToken.cpp
$eosiocpp -o /root/SmartToken/SmartToken.wasm /root/SmartToken/SmartToken.cpp
