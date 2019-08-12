import { Api, JsonRpc } from 'eosjs';
import { JsSignatureProvider } from 'eosjs/dist/eosjs-jssig';
import fs from 'fs';
import path from 'path';
import { assert } from 'chai';
import fetch from 'node-fetch';

const testUser = 'test1';

export const getKeyFile = account => JSON.parse(fs.readFileSync(path.resolve(process.env.ACCOUNTS_PATH, `${account}.json`)).toString())

export const host = () => {
    const h = process.env.NETWORK_HOST;
    const p = process.env.NETWORK_PORT;
    return `http://${h}:${p}`;
};

const rpc = new JsonRpc(host(), { fetch });
export const getEos = (account = testUser) => new Api({ rpc, keyProvider: new JsSignatureProvider([getKeyFile(account).privateKey]) })

export const snooze = ms => new Promise(resolve => setTimeout(resolve, ms));

export async function ensureContractAssertionError(prom, expected_error) {
    try {
        await prom;
        assert(false, 'should have failed');
    }
    catch (err) {
        err.should.include(expected_error);
    }
}

export async function ensurePromiseDoesntThrow(prom) {
    let wasRejected = false;

    try {
        await prom;
    }
    catch (err) {
        wasRejected = true;
    }

    assert.equal(wasRejected, false, 'promise should have resolved');
}

export async function getBalance(tokenContract, owner) {
    const eosInstance = getEos();

    const data = (await eosInstance.getTableRows(true, tokenContract, owner, 'accounts')).rows[0];

    return data ? data.balance.split(' ')[0] : '0';
}

export async function getSupply(tokenContract, symbol) {
    const eosInstance = getEos();

    const data = (await eosInstance.getTableRows(true, tokenContract, symbol, 'stat')).rows[0];

    return data ? data.supply.split(' ')[0] : '0';
}