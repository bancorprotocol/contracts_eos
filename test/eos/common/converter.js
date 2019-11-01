
const { api, rpc, getTableBoundsForSymbol } = require('./utils')

const networkContract = 'thisisbancor'
const networkToken = 'bntbntbntbnt'
const networkTokenSymbol = "BNT"

const multiConverter = 'multiconvert'
const multiToken = 'multi4tokens'
const multiStaking = 'multistaking'
const bntConverter = 'bnt2eoscnvrt'
const bntStaking = 'stakebnt2eos'
const bntRelay = 'bnt2eosrelay'
const bntRelaySymbol = 'BNTEOS'

const getSettings = async function (converter = bntConverter) {
    try {
        const result = await rpc.get_table_rows({
            "code": converter,
            "scope": converter,
            "table": 'settings'
        })
        return result
    } catch (err) {
        throw(err)
    }
}
const getAccount = async function (owner, converterSymbol, reserveSymbol) {
    const reserveHexLE = getTableBoundsForSymbol(reserveSymbol).lower_bound;
    const converterBounds = getTableBoundsForSymbol(converterSymbol);
    const bounds = {
        lower_bound: `0x${converterBounds.lower_bound}${reserveHexLE}`,
        upper_bound: `0x${converterBounds.upper_bound}${reserveHexLE}`,
    }
    try {
        const result = await rpc.get_table_rows({
            "code": multiConverter,
            "scope": owner,
            "key_type" : "i128",
            "table": "accounts",
            "index_position": 2,
            ...bounds
        })
        return result
    } catch (err) {
        throw(err)
    }
}
const getReserve = async function (symbol, converter = bntConverter, scope = converter) {
    try {
        const result = await rpc.get_table_rows({
            "code": converter,
            "scope": scope,
            "table": "reserves",
            "limit": 1,
            "lower_bound": symbol
        })
        return result
    } catch (err) {
        throw(err)
    }
}
const getConverter = async function (symbol) {
    try {
        const result = await rpc.get_table_rows({
            "code": multiConverter,
            "scope": symbol,
            "table": "converters",
            "limit": 1,
            "lower_bound": symbol
        })
        return result
    } catch (err) {
        throw(err)
    }
}
const init = async function (converter = bntConverter, actor = converter, relay = bntRelay, symbol = bntRelaySymbol, fee = 0, smart_enabled = false) {
    try {
        const result = await api.transact({ 
            actions: [{
                account: converter,
                name: "init",
                authorization: [{
                    actor,
                    permission: 'active',
                }],
                data: {
                    smart_contract: relay,
                    smart_currency: `0.00000000 ${symbol}`,
                    smart_enabled: smart_enabled,
                    enabled: true,
                    network: networkContract,
                    require_balance: false,
                    max_fee: 30000,
                    fee: fee
                }
            }]
        }, 
        {
            blocksBehind: 3,
            expireSeconds: 30,
        })
        return result
    } catch(err) {
        throw(err)
    }
}
const delreserve = async function(currency = 'BNT', actor = bntConverter, converter = bntConverter, converterScope = null) {
    try {
        const result = await api.transact({ 
            actions: [{
                account: converter,
                name: "delreserve",
                authorization: [{
                    actor,
                    permission: 'active',
                }],
                data: {
                    ...(converterScope ? { converter: converterScope, currency } : { currency })
                }
            }]
        }, 
        {
            blocksBehind: 3,
            expireSeconds: 30,
        })
        console.log(JSON.stringify(result.processed.action_traces))
        return result
    } catch(err) {
        throw(err)
    }
}
const update = async function(fee = 0, converter = bntConverter, actor = converter, 
                              smart_enabled = true, enabled = true, 
                              require_balance = false) {
    try {
        const result = await api.transact({ 
            actions: [{
                account: converter,
                name: "update",
                authorization: [{
                    actor,
                    permission: 'active',
                }],
                data: {
                    smart_enabled, 
                    enabled, 
                    require_balance, 
                    fee
                }
            }]
        }, 
        {
            blocksBehind: 3,
            expireSeconds: 30,
        })
        return result
    } catch(err) {
        throw(err)
    }
}
const activateStaking = async function(stake_contract = bntStaking, stake_enabled = true, fee = 0, 
                                       converter = bntConverter, actor = converter, smart_enabled = true, 
                                       enabled = true, require_balance = false) {
    try {
        extend = {
            stake_contract,
            stake_enabled
        }
        const result = await api.transact({ 
            actions: [{
                account: converter,
                name: "update",
                authorization: [{
                    actor,
                    permission: 'active',
                }],
                data: {
                    smart_enabled, 
                    enabled, 
                    require_balance, 
                    fee,
                    extend
                }
            }]
        }, 
        {
            blocksBehind: 3,
            expireSeconds: 30,
        })
        return result
    } catch(err) {
        throw(err)
    }
}
const setreserve = async function(precise = true, token = networkToken, 
                                  symbol = networkTokenSymbol, 
                                  converter = bntConverter,
                                  converterScope = null, 
                                  actor = converter, 
                                  ratio = 500000) {
    var precision = 8
    if (!precise)
        precision = 4
    try {
        const result = await api.transact({ 
            actions: [{
                account: converter,
                name: "setreserve",
                authorization: [{
                    actor,
                    permission: 'active',
                }],
                data: {
                    contract: token,
                    currency: `${precision},${symbol}`,
                    ratio,
                    sale_enabled: true,
                    ...(converterScope ? { converter_currency_code: converterScope } : {})
                }
            }]
        }, 
        {
            blocksBehind: 3,
            expireSeconds: 30,
        })
        return result
    } catch(err) {
        throw(err)
    }
}
const setEnabled = async function(actor, enabled = true) {
    const result = await api.transact({ 
        actions: [{
            account: multiConverter,
            name: "setenabled",
            authorization: [{
                actor,
                permission: 'active',
            }], data: { enabled }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
    return result;
}
const setMaxfee = async function(actor, maxfee) {
    const result = await api.transact({ 
        actions: [{
            account: multiConverter,
            name: "setmaxfee",
            authorization: [{
                actor,
                permission: 'active',
            }], data: { maxfee }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
    return result;
}
const setStaking = async function(actor, staking = multiStaking) {
    const result = await api.transact({ 
        actions: [{
            account: multiConverter,
            name: "setstaking",
            authorization: [{
                actor,
                permission: 'active',
            }], data: { staking }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
    return result;
}
const setMultitoken = async function(actor, token = multiToken) {
    const result = await api.transact({ 
        actions: [{
            account: multiConverter,
            name: "setmultitokn",
            authorization: [{
                actor,
                permission: 'active',
            }], data: { multi_token: token }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
    return result;
}
// Creates a converter (MultiConverter sub-converter)
const createConverter = async function(owner, initial_supply, maximum_supply) {
    const result = await api.transact({ 
        actions: [{
            account: multiConverter,
            name: 'create',
            authorization: [{
                actor: owner,
                permission: 'active',
            }],
            data: {
                owner,
                initial_supply,
                maximum_supply
            }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
    return result;
}
const delConverter = async function(converter_currency_code, owner) {
    const result = await api.transact({ 
        actions: [{
            account: multiConverter,
            name: 'close',
            authorization: [{
                actor: owner,
                permission: 'active',
            }],
            data: {
                converter_currency_code
            }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
    console.log(JSON.stringify(result.processed.action_traces))
    return result;
}
const enableConvert = async function(actor, currency, enabled = true) {
    const result = await api.transact({ 
        actions: [{
            account: multiConverter,
            name: "enablecnvrt",
            authorization: [{
                actor,
                permission: 'active',
            }], data: { currency, enabled }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
    return result;
}
const enableStake = async function(actor, currency, enabled = true) {
    const result = await api.transact({ 
        actions: [{
            account: multiConverter,
            name: "enablestake",
            authorization: [{
                actor,
                permission: 'active',
            }], data: { currency, enabled }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
    return result;
}
const updateOwner = async function(actor, currency, owner) {
    const result = await api.transact({ 
        actions: [{
            account: multiConverter,
            name: "updateowner",
            authorization: [{
                actor,
                permission: 'active',
            }],
            data: {
                currency,
                owner
            }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
    return result;
}
const updateFee = async function(actor, currency, fee) {
    const result = await api.transact({ 
        actions: [{
            account: multiConverter,
            name: "updatefee",
            authorization: [{
                actor,
                permission: 'active',
            }],
            data: {
                currency,
                fee
            }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
    return result;
}
const withdraw = async function(owner, quantity, converter_currency_code) {
    const result = await api.transact({ 
        actions: [{
            account: multiConverter,
            name: "withdraw",
            authorization: [{
                actor: owner,
                permission: 'active',
            }],
            data: {
                owner,
                quantity,
                converter_currency_code
            }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
    return result;
}
const fund = async function(owner, quantity) {
    const result = await api.transact({ 
        actions: [{
            account: multiConverter,
            name: "fund",
            authorization: [{
                actor: owner,
                permission: 'active',
            }],
            data: {
                owner,
                quantity
            }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
    return result;
}
module.exports = { init, update, enableStake,
                   activateStaking, setStaking, 
                   setreserve, getReserve, delreserve, 
                   getSettings, setMultitoken, 
                   setMaxfee, updateFee, updateOwner, 
                   setEnabled, enableConvert, getAccount,   
                   getConverter, createConverter,
                   delConverter, withdraw, fund }
