
const { api, rpc } = require('./utils');

const networkContract = 'thisisbancor';
const networkToken = 'bntbntbntbnt';
const networkTokenSymbol = "BNT";

const sysConverter = 'bnt2syscnvrt';
const bntConverter = 'bnt2eoscnvrt';
const multiConverter = 'multiconvert';
const multiTokens = 'multi4tokens';
const bntRelay = 'bnt2eosrelay';
const bntRelaySymbol = 'BNTEOS';
const sysRelaySymbol = 'BNTSYS';

const user = 'bnttestuser1';

const get = async function (token, symbol) {
    try {
        const result = await rpc.get_table_rows({
            "code": token,
            "scope": symbol,
            "table": "stat"
        })
        return result
    } catch (err) {
        throw(err)
    }
}
const create = async function (issuer, token, symbol, precise=true) {
    try {
        var precision = '250000000.00000000'
        if (!precise)
            precision = '250000000.0000'
        const result = await api.transact({ 
            actions: [{
                account: token,
                name: "create",
                authorization: [{
                    actor: token,
                    permission: 'active',
                }],
                data: {
                    issuer: issuer,
                    maximum_supply: `${precision} ${symbol}`
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
const issue = async function (token, issuer, quantity, memo) {
    try {
        const result = await api.transact({ 
            actions: [{
                account: token,
                name: "issue",
                authorization: [{
                    actor: issuer,
                    permission: 'active',
                }],
                data: {
                    to: issuer,
                    quantity: quantity,
                    memo: memo
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
const transfer = async function (token, quantity, to = bntConverter, from = token, memo = 'setup') { 
    try {
        const result = await api.transact({ 
            actions: [{
                account: token,
                name: "transfer",
                authorization: [{
                    actor: from,
                    permission: 'active',
                }],
                data: {
                    from,
                    to,
                    quantity,
                    memo
                }
            }]
        }, 
        {
            blocksBehind: 3,
            expireSeconds: 30,
        })
        return result
    } catch (err) {
        throw(err)
    }
}
const getBalance = async function (user, token, symbol) {
    try {
        const result = await rpc.get_table_rows({
            "code": token,
            "scope": user,
            "table": "accounts",
            "limit": 1,
            "lower_bound": symbol,
        })
        return result
    } catch (err) {
        throw(err)
    }
}
const convertMulti = async function (amount, symbol, targetSymbol, 
                                   converter = multiConverter, 
                                   from = user, min = '0.00000001') {
    try {
        const result = await api.transact({ 
            actions: [{
                account: multiTokens,
                name: "transfer",
                authorization: [{
                    actor: from,
                    permission: 'active',
                }],
                data: {
                    from: from,
                    to: networkContract,
                    quantity: `${amount} ${symbol}`,
                    memo: `1,${converter}:${symbol} ${targetSymbol},${min},${from}`
                }
            }]
        }, 
        {
            blocksBehind: 3,
            expireSeconds: 30,
        })
        return result
    } catch (err) {
        throw(err)
    }
}
const convertRelay = async function (amount, from = user, converter = bntConverter, 
                                     relay = bntRelay, relaySymbol = bntRelaySymbol, 
                                     toSymbol = networkTokenSymbol) 
{ // buy BNT with BNTEOS
    try {
        const minReturn = '0.00000010';
        const result = await api.transact({ 
            actions: [{
                account: relay,
                name: "transfer",
                authorization: [{
                    actor: from,
                    permission: 'active',
                }],
                data: {
                    from: from,
                    to: networkContract,
                    quantity: `${amount} ${relaySymbol}`,
                    memo: `1,${converter} ${toSymbol},${minReturn},${from}`
                }
            }]
        }, 
        {
            blocksBehind: 3,
            expireSeconds: 30,
        })
        return result
    } catch (err) {
        throw(err)
    }
}
const convertBNT = async function (amount, toSymbol = bntRelaySymbol, relay = bntConverter,
                                   from = user, to = from, min = '0.00000001') { // buy BNTEOS with BNT
    try {
        const result = await api.transact({ 
            actions: [{
                account: networkToken,
                name: "transfer",
                authorization: [{
                    actor: from,
                    permission: 'active',
                }],
                data: {
                    from: from,
                    to: networkContract,
                    quantity: `${amount} ${networkTokenSymbol}`,
                    memo: `1,${relay} ${toSymbol},${min},${to}`
                }
            }]
        }, 
        {
            blocksBehind: 3,
            expireSeconds: 30,
        })
        return result
    } catch (err) {
        throw(err)
    }
}
const convertTwice = async function (amount, token, relaySymbol = bntRelaySymbol, relay2symbol = sysRelaySymbol,
                                     relay = bntConverter, relay2 = sysConverter,
                                     from = user, to = user, min = '0.00000001') { // buy BNTSYS with BNTEOS
    try {
        const result = await api.transact({ 
            actions: [{
                account: token,
                name: "transfer",
                authorization: [{
                    actor: from,
                    permission: 'active',
                }],
                data: {
                    from: from,
                    to: networkContract,
                    quantity: `${amount} ${relaySymbol}`,
                    memo: `1,${relay} ${networkTokenSymbol} ${relay2} ${relay2symbol},${min},${to}`
                }
            }]
        }, 
        {
            blocksBehind: 3,
            expireSeconds: 30,
        })
        return result
    } catch (err) {
        throw(err)
    }
}
const convertEOS = async function (amount,  fake=false, relay = bntConverter, relaySymbol = bntRelaySymbol,
                                   from = user, to = user) { // buy BNTEOS with EOS
    try {
        const minReturn = '0.00000001';
        account = 'eosio.token'
        if (fake)
            account = 'fakeos'
        const result = await api.transact({ 
            actions: [{
                account: account,
                name: "transfer",
                authorization: [{
                    actor: from,
                    permission: 'active',
                }],
                data: {
                    from: from,
                    to: networkContract,
                    quantity: `${amount} EOS`,
                    memo: `1,${relay} ${relaySymbol},${minReturn},${to}`
                }
            }]
        }, 
        {
            blocksBehind: 3,
            expireSeconds: 30,
        })
        return result
    } catch (err) {
        throw(err)
    }
}
const convertSYS = async function (amount, token = 'fakeos', relay = bntConverter, 
                                   relaySymbol = bntRelaySymbol, from = user, to = user) { // buy BNTEOS with SYS
    try {
        const minReturn = '0.00000001';
        const result = await api.transact({ 
            actions: [{
                account: token,
                name: "transfer",
                authorization: [{
                    actor: from,
                    permission: 'active',
                }],
                data: {
                    from: from,
                    to: networkContract,
                    quantity: `${amount} SYS`,
                    memo: `1,${relay} ${relaySymbol},${minReturn},${to}`
                }
            }]
        }, 
        {
            blocksBehind: 3,
            expireSeconds: 30,
        })
        return result
    } catch (err) {
        throw(err)
    }
}
module.exports = {
    get, issue, create,
    transfer, getBalance, 
    convertTwice, convertBNT,
    convertEOS, convertSYS, 
    convertRelay, convertMulti
}
