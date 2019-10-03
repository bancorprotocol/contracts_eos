#!/bin/bash
nodeos -d ./nodeos/data --config-dir ./nodeos/config  --max-transaction-time=1000 --delete-all-blocks --contracts-console --verbose-http-errors --http-validate-host=false --plugin eosio::http_plugin --plugin eosio::producer_plugin --plugin eosio::producer_api_plugin --plugin eosio::chain_api_plugin --plugin eosio::history_api_plugin -p eosio -e 2>stderr &
sleep 5
curl -X POST http://127.0.0.1:8888/v1/producer/schedule_protocol_feature_activations -d '{"protocol_features_to_activate": ["0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd"]}' | jq