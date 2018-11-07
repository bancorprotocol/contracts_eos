require("babel-core/register");
require("babel-polyfill");
import Eos from 'eosjs';
import { assert } from 'chai';
import 'mocha';
const fs = require('fs');
const path = require('path');

const host = () => {
    const h = process.env.NETWORK_HOST;
    const p = process.env.NETWORK_PORT;
    return `http://${h}:${p}`;
};

describe('BancorNetwork Contract', () => {
    const converter = 'cnvtaa';
    const converter2 = 'cnvtbb';
    const networkContract = 'bancornetwrk';
    const networkTokenSymbol = "BNT";
    const networkToken = 'bnt';
    const tokenSymbol = "TKNA";
    const tokenSymbol2 = "TKNB";
    const testUser = 'test1';
    const tokenContract= 'aa';
    const keyFile = JSON.parse(fs.readFileSync(path.resolve(process.env.ACCOUNTS_PATH, `${testUser}.json`)).toString());
    const codekey = keyFile.privateKey;
    const _self = Eos({ httpEndpoint:host(), keyProvider:codekey });
    const _selfopts = { authorization:[`${testUser}@active`] };
    
    it('simple convert', done => {
        var minReturn = 0.100;
        _self.contract(networkToken).then(async token => {
            return await token.transfer({ from: testUser, to: networkContract, quantity: `2.0000000000 ${networkTokenSymbol}`, memo: `1,${converter} ${tokenSymbol},${minReturn},${testUser}` }, _selfopts);
        }).then((res) => {
            var events = res.processed.action_traces[0].inline_traces[2].inline_traces[1].console.split("\n");
            console.log(events)
            var convertEvent = JSON.parse(events[0]);
            assert.equal(convertEvent.return, "199996000", "unexpected conversion result");
            // console.log("result",jObj.target_amount);
            done();
        }).catch((err) => done(err));
    });

    it('2 hop convert', done => {
        var minReturn = 0.100;
        _self.contract(tokenContract).then(async token => {
            return await token.transfer({ from: testUser, to: networkContract, quantity: `1.00000000 ${tokenSymbol}`, memo: `1,${converter} ${networkTokenSymbol} ${converter2} ${tokenSymbol2},${minReturn},${testUser}` }, _selfopts);
        }).then((res) => {
            var events = res.processed.action_traces[0].inline_traces[2].inline_traces[1].console.split("\n");
            console.log(events)
            var convertEvent = JSON.parse(events[0]);
            
            assert.equal(convertEvent.return, "10000299998", "unexpected conversion result");
            // console.log("action console", res.processed.action_traces[0].inline_traces);
            events = res.processed.action_traces[0].inline_traces[2].inline_traces[2].inline_traces[2].inline_traces[1].console.split("\n");
            console.log(events)
            convertEvent = JSON.parse(events[0]);
            assert.equal(convertEvent.return, "100001999", "unexpected conversion result");
            done();
        }).catch((err) => done(err));
    });
});
