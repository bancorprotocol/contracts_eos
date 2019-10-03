#!/bin/bash
shopt -s expand_aliases
source ~/.bash_profile
source ~/.profile
source ~/.bashrc

GREEN='\033[0;32m'
NC='\033[0m'

declare -a contracts=("MultiConverter" "MultiToken" "BancorConverter" "BancorNetwork" "Token" "BancorX" "XTransferRerouter")
for contract in "${contracts[@]}"
do
    echo -e "${GREEN}Compiling $contract...${NC}"
    eosio-cpp ./contracts/eos/$contract/$contract.cpp -o ./contracts/eos/$contract/$contract.wasm --abigen -I.
done
