cmake_minimum_required(VERSION 3.5)
project(BancorNetwork VERSION 1.0.0)

find_package(eosio.cdt)

### Only generate the wasm
#add_executable( BancorNetwork.wasm BancorNetwork.cpp )

### Generate the wasm and abi
add_contract( BancorX BancorX BancorX.cpp )