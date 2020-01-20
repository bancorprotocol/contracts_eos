
const { api, rpc } = require('./utils');


const config = require('../../../config/accountNames.json')

const networkContract = config.BANCOR_NETWORK_ACCOUNT;
const networkToken = config.BNT_TOKEN_ACCOUNT;

const user = config.MASTER_ACCOUNT;
const multiConverter = config.MULTI_CONVERTER_ACCOUNT
const multiToken = config.MULTI_TOKEN_ACCOUNT
const bntConverter = multiConverter;
const bntRelay = multiToken;
const sysConverter = 'bnt2syscnvrt';
const bntRelaySymbol = 'BNTEOS';
const sysRelaySymbol = 'BNTSYS';
const networkTokenSymbol = "BNT";


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
        let precision = '250000000.00000000'
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
const convertMulti = async function(amount, symbol, targetSymbol, 
                                    converter = multiConverter, 
                                    from = user, min = '0.00000001',
                                    affiliate = null, affiliateFee = null) {
    try {
        let memo = `1,${converter}:${symbol} ${targetSymbol},${min},${from}`
        if (affiliate)
            memo += `,${affiliate},${affiliateFee}`
        
        const result = await api.transact({ 
            actions: [{
                account: multiToken,
                name: "transfer",
                authorization: [{
                    actor: from,
                    permission: 'active',
                }],
                data: {
                    from: from,
                    to: networkContract,
                    quantity: `${amount} ${symbol}`,
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
const convertRelay = async function (amount, from = user, converter = bntConverter, 
                                     relay = bntRelay, relaySymbol = bntRelaySymbol, 
                                     toSymbol = networkTokenSymbol) { // buy BNT with BNTEOS
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
                    memo: `1,${converter}:${relaySymbol} ${toSymbol},${minReturn},${from}`
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
                                   from = user, to = from, affiliate = null, 
                                   affiliateFee = null, min = '0.00000001') {
    try {
        let memo = `1,${relay}:BNTEOS ${toSymbol},${min},${to}`

        if (affiliate)
            memo += `,${affiliate},${affiliateFee}`

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
                    memo: `1,${relay}:BNTEOS ${networkTokenSymbol} ${relay2} ${relay2symbol},${min},${to}`
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
const convertEOS = async function (amount, fake=false, relay = bntConverter, relaySymbol = bntRelaySymbol,
                                   from = user, to = user, affiliate = null, affiliateFee = null, min = null) { // buy BNTEOS with EOS
    try {
        
        let minReturn = '0.00000001'; 
        if (min) minReturn = min
        let account = 'eosio.token'
        let memo = `1,${relay} ${relaySymbol},${minReturn},${to}`

        if (fake)
            account = 'fakeos'

        if (affiliate)
            memo += `,${affiliate},${affiliateFee}`
    
        const result = await api.transact({ 
            actions: [{
                account: account,
                name: "transfer",
                authorization: [{
                    actor: from,
                    permission: 'active',
                }],
                data: {
                    from,
                    to: networkContract,
                    quantity: `${amount} EOS`,
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
