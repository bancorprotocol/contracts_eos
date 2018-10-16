cmake_minimum_required(VERSION 3.5)
project(SmartToken VERSION 1.0.0)

find_package(eosio.cdt)

### Only generate the wasm
#add_executable( SmartToken.wasm SmartToken.cpp )

### Generate the wasm and abi
add_contract( SmartToken SmartToken SmartToken.cpp )