#!/bin/bash

./scripts/kill_nodeos.sh
./scripts/start_nodeos.sh

sleep 2
./scripts/deploy.sh
