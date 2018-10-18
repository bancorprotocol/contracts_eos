require("babel-core/register");
require("babel-polyfill");
import Eos from 'eosjs';
import 'mocha';
const fs = require('fs');
const path = require('path');

const host = () => {
    const h = process.env.NETWORK_HOST;
    const p = process.env.NETWORK_PORT;
    return `http://${h}:${p}`;
};

describe('BancorConverter Contract', () => {
    const code = 'bancornetwrk';
    const keyFile = JSON.parse(fs.readFileSync(path.resolve(process.env.ACCOUNTS_PATH, `${code}.json`)).toString());
    const codekey = keyFile.privateKey;
    const _self = Eos({ httpEndpoint:host(), keyProvider:codekey });
    const _selfopts = { authorization:[`${code}@active`] };
});
