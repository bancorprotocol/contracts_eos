#!/bin/bash

eosio-cpp BancorConverter.cpp
cleos set contract bancorcnvrtr . BancorConverter.wasm BancorConverter.abi
