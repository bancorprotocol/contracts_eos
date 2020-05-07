const {
    api,
    rpc,
    getTableBoundsForName,
    getTableBoundsForSymbol
 } = require('./utils');
const config = require('../../../config/accountNames.json')

const defaultAuth = {
    actor: config.MASTER_ACCOUNT,
    permission: 'active'
}

async function addConverter(converterRegistryAccount, converterAccount, poolTokenSymbolCode, authorization = defaultAuth) {
    return api.transact({ 
        actions: [{
            account: converterRegistryAccount,
            name: "addconverter",
            authorization: [authorization],
            data: {
                converter_account: converterAccount,
                pool_token_code: poolTokenSymbolCode
            }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
}

async function rmConverter(converterRegistryAccount, converterAccount, poolTokenSymbolCode, authorization = defaultAuth) {
    return api.transact({ 
        actions: [{
            account: converterRegistryAccount,
            name: "rmconverter",
            authorization: [authorization],
            data: {
                converter_account: converterAccount,
                pool_token_code: poolTokenSymbolCode
            }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
}

async function getConverter(converterRegistryAccount, converterAccount, poolTokenSymbol) {
    const converterAccountLowerBound = getTableBoundsForName(converterAccount).lower_bound;
    const poolTokenSymbolBounds = getTableBoundsForSymbol(poolTokenSymbol);
    const bounds = {
        lower_bound: `0x${poolTokenSymbolBounds.lower_bound}${converterAccountLowerBound}`,
        upper_bound: `0x${poolTokenSymbolBounds.upper_bound}${converterAccountLowerBound}`,
    }

    return rpc.get_table_rows({
        "code": converterRegistryAccount,
        "scope": converterRegistryAccount,
        "key_type" : "i128",
        "table": "converters",
        "index_position": 2,
        ...bounds
    })
}

async function getLiquidityPool(converterRegistryAccount, liquidityPoolAccount, poolTokenSymbol) {
    const liquidityPoolAccountLowerBound = getTableBoundsForName(liquidityPoolAccount).lower_bound;
    const poolTokenSymbolBounds = getTableBoundsForSymbol(poolTokenSymbol);
    const bounds = {
        lower_bound: `0x${poolTokenSymbolBounds.lower_bound}${liquidityPoolAccountLowerBound}`,
        upper_bound: `0x${poolTokenSymbolBounds.upper_bound}${liquidityPoolAccountLowerBound}`,
    }

    return rpc.get_table_rows({
        "code": converterRegistryAccount,
        "scope": converterRegistryAccount,
        "key_type" : "i128",
        "table": "liqdtypools",
        "index_position": 2,
        ...bounds
    })
}

async function getPoolToken(converterRegistryAccount, poolTokenAccount, poolTokenSymbol) {
    const poolTokenAccountLowerBound = getTableBoundsForName(poolTokenAccount).lower_bound;
    const poolTokenSymbolBounds = getTableBoundsForSymbol(poolTokenSymbol);
    const bounds = {
        lower_bound: `0x${poolTokenSymbolBounds.lower_bound}${poolTokenAccountLowerBound}`,
        upper_bound: `0x${poolTokenSymbolBounds.upper_bound}${poolTokenAccountLowerBound}`,
    }

    return rpc.get_table_rows({
        "code": converterRegistryAccount,
        "scope": converterRegistryAccount,
        "key_type" : "i128",
        "table": "pooltokens",
        "index_position": 2,
        ...bounds
    })
}

async function getConvertiblePairs(converterRegistryAccount, fromTokenSym, converterAccount, poolTokenSymbol) {
    const converterAccountLowerBound = getTableBoundsForName(converterAccount).lower_bound;
    const poolTokenSymbolBounds = getTableBoundsForSymbol(poolTokenSymbol);
    const bounds = {
        lower_bound: `0x${poolTokenSymbolBounds.lower_bound}${converterAccountLowerBound}`,
        upper_bound: `0x${poolTokenSymbolBounds.upper_bound}${converterAccountLowerBound}`,
    }

    return rpc.get_table_rows({
        "code": converterRegistryAccount,
        "scope": fromTokenSym,
        "key_type" : "i128",
        "table": "cnvrtblpairs",
        "index_position": 2,
        ...bounds
    })
}


module.exports = {
    addConverter,
    rmConverter,
    getConverter,
    getLiquidityPool,
    getPoolToken,
    getConvertiblePairs
};
