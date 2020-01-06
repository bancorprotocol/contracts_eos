#!/bin/bash
nodeos -p eosio -e \
  --data-dir ./nodeos/data \
  --config-dir ./nodeos/config  \
  --plugin eosio::http_plugin \
  --plugin eosio::chain_plugin \
  --plugin eosio::chain_api_plugin \
  --plugin eosio::producer_plugin \
  --plugin eosio::producer_api_plugin \
  --plugin eosio::history_plugin \
  --plugin eosio::history_api_plugin \
  --plugin eosio::state_history_plugin \
  --http-server-address=0.0.0.0:8888 \
  --access-control-allow-origin=* \
  --http-validate-host=false \
  --max-transaction-time=1000 \
  --replay-blockchain \
  --hard-replay-blockchain \
  --verbose-http-errors \
  --disable-replay-opts \
  --delete-all-blocks \
  --contracts-console \
  --filter-on=* \
  --filter-out=eosio:onblock: \
  --trace-history \
  --chain-state-history 2>stderr &

sleep 1
curl -X POST http://127.0.0.1:8888/v1/producer/schedule_protocol_feature_activations -d '{"protocol_features_to_activate": ["0ec7e080177b2c02b278d5088611686b49d739925a92d9bfcacd7fc6b74053bd"]}' | jq
