cmake_minimum_required(VERSION 3.5)
project(BancorConverter VERSION 1.0.0)

find_package(eosio.cdt)

### Only generate the wasm
#add_executable( BancorConverter.wasm BancorConverter.cpp )

### Generate the wasm and abi
add_contract( BancorConverter BancorConverter BancorConverter.cpp )