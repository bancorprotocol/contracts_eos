const { api } = require('./utils');
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


module.exports = {
    addConverter
};
