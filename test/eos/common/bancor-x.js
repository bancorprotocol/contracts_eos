const { api } = require('./utils');
const config = require('../../../config/accountNames.json')

const networkTokenSymbol = "BNT";
const user = config.MASTER_ACCOUNT;
const bancorXContract = config.BANCOR_X_ACCOUNT;


async function enablext(enable) {
    return api.transact({ 
        actions: [{
            account: bancorXContract,
            name: "enablext",
            authorization: [{
                actor: bancorXContract,
                permission: 'active',
            }],
            data: {
                enable
            }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
}


async function enablerpt(enable) {
    return api.transact({ 
        actions: [{
            account: bancorXContract,
            name: "enablerpt",
            authorization: [{
                actor: bancorXContract,
                permission: 'active',
            }],
            data: {
                enable
            }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
}


async function addreporter(reporter) {
    return api.transact({ 
        actions: [{
            account: bancorXContract,
            name: "addreporter",
            authorization: [{
                actor: bancorXContract,
                permission: 'active',
            }],
            data: {
                reporter
            }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
}


async function rmreporter(reporter) {
    return api.transact({ 
        actions: [{
            account: bancorXContract,
            name: "rmreporter",
            authorization: [{
                actor: bancorXContract,
                permission: 'active',
            }],
            data: {
                reporter
            }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
}


async function update(
    { min_reporters, min_limit, limit_inc, max_issue_limit, max_destroy_limit },
    authorization = { actor: bancorXContract, permission: 'active' }
    ) {
    return api.transact({ 
        actions: [{
            account: bancorXContract,
            name: "update",
            authorization: [authorization],
            data: {
                min_reporters,
                min_limit,
                limit_inc,
                max_issue_limit,
                max_destroy_limit
            }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
}

async function reporttx({ tx_id, reporter, target = user, quantity = `2.00000000 ${networkTokenSymbol}`, memo = 'enjoy your BNTs', data = 'txHash', blockchain = 'eth', x_transfer_id = 0 }) {
    return api.transact({ 
        actions: [{
            account: bancorXContract,
            name: "reporttx",
            authorization: [{
                actor: reporter,
                permission: 'active',
            }],
            data: {
                tx_id,
                x_transfer_id,
                reporter,
                target,
                quantity,
                memo,
                data,
                blockchain
            }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
}

module.exports = {
    enablext,
    reporttx,
    enablerpt,
    addreporter,
    rmreporter,
    update
};
