const { createDfuseClient } = require("@dfuse/client")
const { name, symbol_code } = require("eos-common");

global.fetch = require("node-fetch")
global.WebSocket = require("ws")

const client = createDfuseClient({ apiKey: process.env.DFUSE_API_KEY, network: "mainnet" })

async function main() {
    const results = await client.stateTableScopes("bancorcnvrtr", "converters");

    for ( const scope of results.scopes ) {
        const symcode = symbol_code( name(scope).value ).to_string();
        console.log(symcode);
    }
    client.release()
}

main();