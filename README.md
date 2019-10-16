# Bancor Protocol Contracts v1.2 (beta)

Bancor is a decentralized liquidity network that provides users with a simple, low-cost way to buy and sell tokens. Bancor’s open-source protocol empowers tokens with built-in convertibility directly through their smart contracts, allowing integrated tokens to be instantly converted for one another, without needing to match buyers and sellers in an exchange. The Bancor Wallet enables automated token conversions directly from within the wallet, at prices that are more predictable than exchanges and resistant to manipulation. To convert tokens instantly, including ETH, EOS, DAI and more, visit the [Bancor Web App](https://www.bancor.network/communities/5a780b3a287443a5cdea2477?utm_source=social&utm_medium=github&utm_content=readme), join the [Bancor Telegram group](https://t.me/bancor) or read the Bancor Protocol™ [Whitepaper](https://www.bancor.network/whitepaper) for more information.

## Overview
The Bancor protocol represents the first technological solution for the classic problem in economics known as the “Double Coincidence of Wants”, in the domain of asset exchange. For barter, the coincidence of wants problem was solved through money. For money, exchanges still rely on labor, via bid/ask orders and trade between external agents, to make markets and supply liquidity. 

Through the use of smart-contracts, Smart Tokens can be created that hold one or more other tokens as connectors. Tokens may represent existing national currencies or other types of assets. By using a connector token model and algorithmically-calculated conversion rates, the Bancor Protocol creates a new type of ecosystem for asset exchange, with no central control. This decentralized hierarchical monetary system lays the foundation for an autonomous decentralized global exchange with numerous and substantial advantages.

## Disclaimer

Bancor is a work in progress. Make sure you understand the risks before using it.

# Contracts

Bancor protocol is implemented using multiple contracts. The main ones are a version of eosio.token contract, BancorNetwork and BancorConverter. 

BancorConverter is responsible for converting between a specific token and its own reserves.
BancorNetwork is the entry point for any token to any token conversion.

There are also MultiToken and MultiConverter implementations which allow conversions with numerous several smart tokens (without deploying a separate smart contract for each converter (BancorConverter).

In order to execute a conversion, the caller needs to transfer tokens to the BancorNetwork contract with specific conversion instructions in the transfer memo.

See each contract for a description and general usage information.

## Setup:
- make sure you have node.js+npm installed globally
- make sure you have both eosio installed and the eosio.CDT globally installed (via `apt` or `brew`), or in order to compile latest eosio.contracts, masters cloned from git, built and installed inside the user home directory.
- `npm install` from the root project directory.

## Testing
Tests are included that may be executed using `npm run test` commands. Legacy tests using the `funguy` (legacy `zeus`) SDK may be found in older commits for historical purposes. Additionally included is a convenience script (`chmod u+x` it or run with `bash`) for compiling contracts and deploying on a fresh `nodeos` instance loaded with eosio.contracts binaries, 1.7.0 latest stable release on 10/16/19:

## Running:
- if you already HAVE `nodeos` running, run `npm run restart`
- if you don't have the contracts compiled before the above, run `npm run cstart`
- "restart" and "cstart" will also run tests for you
- if you DON'T already have `nodeos`running, run `npm run start`

A local folder called "nodeos" will be created storing the "config" and "data" related to your last deployment session. 
All nodeos console output will be written to a local file called "stderr".

### Prerequisite Software
* Node.js v8.11.4+
* npm v6.4.1+
* eosio v1.8.4
* eosio.cdt v1.6.2

## Collaborators

* **[Tal Muskal](https://github.com/tmuskal)**
* **[Yudi Levi](https://github.com/yudilevi)**
* **[Or Dadosh](https://github.com/ordd)**
* **[Yuval Weiss](https://github.com/yuval-weiss)**
* **[Rick Tobacco](https://github.com/ricktobacco)**


## License

Bancor Protocol is open source and distributed under the Apache License v2.0
