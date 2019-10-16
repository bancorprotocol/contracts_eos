
const { api, rpc, getTableBoundsForName, getTableBoundsForSymbol } = require('./utils');

const multiConverter = 'multiconvert'
const multiStaking = 'multistaking'
const bntRelaySymbol = 'BNTEOS'
const defaultAmt = '1.00000000'
const user1 = 'bnttestuser1'
const user2 = 'bnttestuser2'

const setConfig = async function (refund_delay_sec = 1,
                                  start_vote_after = 1,
                                  converter = multiConverter, 
                                  enabled = true ) {
    try {
        const result = await api.transact({ 
            actions: [{
                account: multiStaking,
                name: "setconfig",
                authorization: [{
                    actor: multiStaking,
                    permission: 'active',
                }],
                data: {
                    refund_delay_sec,
                    start_vote_after,
                    converter, enabled
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
const vote = async function (amount = defaultAmt, owner = user1,
                             property = 'fee', symbol = bntRelaySymbol) {
    try {
        const result = await api.transact({ 
            actions: [{
                account: multiStaking,
                name: "vote",
                authorization: [{
                    actor: owner,
                    permission: 'active',
                }],
                data: {
                    property,
                    value: `${amount} ${symbol}`,
                    owner
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
const stake = async function (amount = defaultAmt, symbol = bntRelaySymbol,
                              from = user1, to = from, transfer = false) {
    try {
        const result = await api.transact({ 
            actions: [{
                account: multiStaking,
                name: "delegate",
                authorization: [{ // TODO, test "to" permission for delegates show_payer
                    actor: from,
                    permission: 'active',
                }],
                data: {
                    amount: `${amount} ${symbol}`,
                    from,
                    to,
                    transfer
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
const unstake = async function (amount = defaultAmt, symbol = bntRelaySymbol,
                                from = user1, to = from, unstake = false) {
    try {
        const result = await api.transact({ 
            actions: [{
                account: multiStaking,    
                name: "undelegate",
                authorization: [{
                    actor: from,
                    permission: 'active',
                }],
                data: {
                    from,
                    to,
                    unstake,
                    amount: `${amount} ${symbol}`
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
const refund = async function (by = user2, owner = user1, symbl = bntRelaySymbol) {
    try {
        const result = await api.transact({ 
            actions: [{
                account: multiStaking,
                name: "refund",
                authorization: [{
                    actor: by,
                    permission: 'active',
                }],
                data: { owner, symbl }
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
const getRefunds = async function (user = user1, symbol = bntRelaySymbol) {
    try {
        const result = await rpc.get_table_rows({
            "code": multiStaking,
            "scope": symbol,
            "table": "refunds",
            "limit": 1,
            "lower_bound": user,
        })
        return result
    } catch (err) {
        throw(err)
    }
}
const getWeights  = async function (property = 'fee', symbol = bntRelaySymbol) {
    try {
        const result = await rpc.get_table_rows({
            "code": multiStaking,
            "scope": property,
            "table": "weights",
            "lower_bound": symbol
        })
        return result
    } catch (err) {
        throw(err)
    }
}
const getUserStake = async function (user = user1, symbol = bntRelaySymbol) {
    try {
        const result = await rpc.get_table_rows({
            "code": multiStaking,
            "scope": user,
            "table": "stakes",
            "lower_bound": symbol
        })
        return result
    } catch (err) {
        throw(err)
    }
}
const getVotes = async function (owner = user1, property = 'fee', 
                                 symbol = bntRelaySymbol) {

    const symbolHexLE = getTableBoundsForSymbol(symbol).lower_bound;
    const propertyBounds = getTableBoundsForName(property);
    const bounds = {
        lower_bound: `0x${propertyBounds.lower_bound}${symbolHexLE}`,
        upper_bound: `0x${propertyBounds.upper_bound}${symbolHexLE}`,
    }
    try {
        const result = await rpc.get_table_rows({
            "code" : multiStaking,
            "scope" : owner,
            "key_type" : "i128", 
            "table": "votes",
            "index_position": 2,
            ...bounds
        })
        return result
    } catch (err) {
        throw(err)
    }
}
const getDelegates = async function (delegator = user1, delegate = user1,
                                     symbol = bntRelaySymbol) {

    const symbolHexLE = getTableBoundsForSymbol(symbol).lower_bound;
    const delegateBounds = getTableBoundsForName(delegate);
    const bounds = {
        lower_bound: `0x${delegateBounds.lower_bound}${symbolHexLE}`,
        upper_bound: `0x${delegateBounds.upper_bound}${symbolHexLE}`,
    }
    try {
        const result = await rpc.get_table_rows({
            "code" : multiStaking,
            "scope" : delegator,
            "key_type" : "i128",
            "show_payer" : true, 
            "table": "delegates",
            "index_position": 2,
            ...bounds
        })
        return result
    } catch (err) {
        throw(err)
    }
}
module.exports = {
    stake, unstake, setConfig,
    getVotes, getUserStake,
    getDelegates, getWeights,
    vote, refund, getRefunds,
}
