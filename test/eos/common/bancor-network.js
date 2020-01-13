const { api } = require('./utils')


const setNetworkToken = async function(network_token, account, actor = account) {
    const result = await api.transact({ 
        actions: [{
            account,
            name: "setnettoken",
            authorization: [{
                actor,
                permission: 'active',
            }],
            data: { network_token }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
    return result;
}

const setMaxAffiliateFee = async function(actor, max_affiliate_fee, account = actor) {
    const result = await api.transact({ 
        actions: [{
            account,
            name: "setmaxfee",
            authorization: [{
                actor,
                permission: 'active',
            }],
            data: { max_affiliate_fee }
        }]
    }, 
    {
        blocksBehind: 3,
        expireSeconds: 30,
    })
    return result;
}

module.exports = {
    setNetworkToken,
    setMaxAffiliateFee
};
