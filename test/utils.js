const Eos = require('eosjs');
const fs = require('fs');
const path = require('path');
import { assert } from 'chai';

const testUser = 'test1';

const getKeyFile = account => JSON.parse(fs.readFileSync(path.resolve(process.env.ACCOUNTS_PATH, `${account}.json`)).toString())

const getEos = (account=testUser) => Eos({ httpEndpoint: host(), keyProvider: getKeyFile(account).privateKey })

async function ensureContractAssertionError(prom, expected_error) {
    try {
        await prom;
        assert(false, 'should have failed');
    }
    catch (err) {
        err.should.include(expected_error);
    }
}

async function ensurePromiseDoesntThrow(prom) {
    let wasRejected = false;

    try {
        await prom;
    }
    catch (err) {
        wasRejected = true;
    }

    assert.equal(wasRejected, false, 'promise should have resolved');
}

const host = () => {
    const h = process.env.NETWORK_HOST;
    const p = process.env.NETWORK_PORT;
    return `http://${h}:${p}`;
};

const snooze = ms => new Promise(resolve => setTimeout(resolve, ms));

export async function getBalance(tokenContract, owner) {
    const eosInstance = getEos();

    const data = (await eosInstance.getTableRows(true, tokenContract, owner, 'accounts')).rows[0];

    return data ? data.balance.split(' ')[0] : '0';
}

module.exports ={
    getEos,
    getKeyFile,
    ensureContractAssertionError,
    ensurePromiseDoesntThrow,
    host,
    snooze,
    getBalance
}