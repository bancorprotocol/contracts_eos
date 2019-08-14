
const { api, rpc } = require('./utils');

const networkContract = 'thisisbancor';
const networkToken = 'bntbntbntbnt';
const networkTokenSymbol = "BNT";

const bntConverter = 'bnt2eoscnvrt';
const bntRelay = 'bnt2eosrelay';
const bntRelaySymbol = 'BNTEOS';

const getSettings = async function (converter = bntConverter) {
    try {
        const result = await rpc.get_table_rows({
            "code": converter,
            "scope": converter,
            "table": "settings",
            //"limit": 1,
            //"lower_bound": pk,
            //"upper_bound": pk+1
        })
        return result
    } catch (err) {
        throw(err)
    }
}
const getReserve = async function (symbol, converter = bntConverter) {
    try {
        const result = await rpc.get_table_rows({
            "code": converter,
            "scope": converter,
            "table": "reserves",
            "limit": 1,
            "lower_bound": symbol
        })
        return result
    } catch (err) {
        throw(err)
    }
}
const init = async function (converter = bntConverter, auth = converter, relay = bntRelay, symbol = bntRelaySymbol) {
    try {
        const result = await api.transact({ 
            actions: [{
                account: converter,
                name: "init",
                authorization: [{
                    actor: auth,
                    permission: 'active',
                }],
                data: {
                    smart_contract: relay,
                    smart_currency: `0.00000000 ${symbol}`,
                    smart_enabled: false,
                    enabled: true,
                    network: networkContract,
                    require_balance: false,
                    max_fee: 30000,
                    fee: 0
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
const update = async function(fee = 0, converter = bntConverter, auth = converter, 
                              smart_enabled = true, enabled = true, 
                              require_balance = false) {
    try {
        const result = await api.transact({ 
            actions: [{
                account: converter,
                name: "update",
                authorization: [{
                    actor: auth,
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

const setreserve = async function(precise=true, token = networkToken, symbol = networkTokenSymbol, converter = bntConverter) {
    var precision = '0.00000000'
    if (!precise)
        precision = '0.0000'
    try {
        const result = await api.transact({ 
            actions: [{
                account: converter,
                name: "setreserve",
                authorization: [{
                    actor: converter,
                    permission: 'active',
                }],
                data: {
                    contract: token,
                    currency: `${precision} ${symbol}`,
                    ratio: 500000,
                    p_enabled: true
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
module.exports = { init, update, setreserve, getReserve, getSettings }
