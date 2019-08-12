#!/bin/bash
./scripts/kill_nodeos.sh

nodeos -e -p eosio --max-transaction-time=1000 --http-validate-host=false --delete-all-blocks --contracts-console --plugin eosio::chain_api_plugin --plugin eosio::history_api_plugin --verbose-http-errors --plugin eosio::producer_plugin --plugin eosio::http_plugin