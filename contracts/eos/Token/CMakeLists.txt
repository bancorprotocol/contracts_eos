cmake_minimum_required(VERSION 3.5)
project(Token VERSION 1.0.0)

find_package(eosio.cdt)

### Only generate the wasm
#add_executable( Token.wasm Token.cpp )

### Generate the wasm and abi
add_contract( Token Token Token.cpp )