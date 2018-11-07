cmake_minimum_required(VERSION 3.5)
project(Token VERSION 1.0.0)

find_package(eosio.cdt)

### Only generate the wasm
#add_executable( XTransferRerouter.wasm XTransferRerouter.cpp )

### Generate the wasm and abi
add_contract( XTransferRerouter XTransferRerouter XTransferRerouter.cpp )