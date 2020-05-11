#!/bin/bash
eosiocpp() {
    COMMAND="eosio-cpp $*"
    docker run --rm --name eosio.cdt_v1.7.0 -v $HOME/git/contracts_eos:/project eostudio/eosio.cdt:v1.7.0 /bin/bash -c "$COMMAND"
}

PROJECT_PATH=./project

GREEN='\033[0;32m'
NC='\033[0m'

for contract in "BancorConverter" "BancorNetwork" "Token" "BancorX" "XTransferRerouter"
do
    echo -e "${GREEN}Compiling $contract...${NC} "
    eosiocpp $PROJECT_PATH/contracts/eos/$contract/$contract.cpp -o $PROJECT_PATH/contracts/eos/$contract/$contract.wasm --abigen -I.
done
