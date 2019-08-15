#!/bin/bash
shopt -s expand_aliases
source ~/.bash_profile

GREEN='\033[0;32m'
NC='\033[0m'

declare -a contracts=("BancorConverter" "BancorNetwork" "Token" "BancorX" "XTransferRerouter")
for contract in "${contracts[@]}"
do
    echo -e "${GREEN}Compiling $contract...${NC}"
    eosio-cpp ./contracts/eos/$contract/$contract.cpp -o ./contracts/eos/$contract/$contract.wasm
done
