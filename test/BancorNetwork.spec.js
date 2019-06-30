require("babel-core/register");
require("babel-polyfill");
import Eos from 'eosjs';
import { assert } from 'chai';
import 'mocha';
import { ensureContractAssertionError, getEos } from './utils';
import { ERRORS } from './constants';
const fs = require('fs');
const path = require('path');
const BigNumber = require('bignumber.js');

const host = () => {
    const h = process.env.NETWORK_HOST;
    const p = process.env.NETWORK_PORT;
    return `http://${h}:${p}`;
};

describe('BancorNetwork Contract', () => {
    const converter = 'cnvtaa';
    const converter2 = 'cnvtbb';
    const networkContract = 'thisisbancor';
    const bntConverter = 'bnt2eoscnvrt';
    const networkTokenSymbol = "BNT";
    const networkToken = 'bnt';
    const tokenSymbol = "TKNA";
    const tokenSymbol2 = "TKNB";
    const testUser1 = 'test1';
    const testUser2 = 'test2';
    const tokenContract= 'aa';
    const keyFile = JSON.parse(fs.readFileSync(path.resolve(process.env.ACCOUNTS_PATH, `${testUser1}.json`)).toString());
    const codekey = keyFile.privateKey;
    const _self = Eos({ httpEndpoint:host(), keyProvider:codekey });
    const _selfopts = { authorization:[`${testUser1}@active`] };
    
    it('simple convert', async function() {
        const minReturn = 0.100;
        const amount = '2.0000000000'
        
        const initialFromTokenReserveBalance = (await _self.getTableRows(true, networkToken, converter, 'accounts')).rows[0].balance.split(' ')[0];
        const initialToTokenReserveBalance = (await _self.getTableRows(true, tokenContract, converter, 'accounts')).rows[0].balance.split(' ')[0];
    
        const token = await _self.contract(networkToken)
        const res = await token.transfer({ from: testUser1, to: networkContract, quantity: `${amount} ${networkTokenSymbol}`, memo: `1,${converter} ${tokenSymbol},${minReturn},${testUser1}` }, _selfopts);
        const events = res.processed.action_traces[0].inline_traces[2].inline_traces[1].console.split("\n");

        const convertEvent = JSON.parse(events[0]);
        const toTokenPriceDataEvent = JSON.parse(events[1]);
        const fromTokenPriceDataEvent = JSON.parse(events[2]);

        assert.equal(convertEvent.return, 1.99996000, "unexpected conversion return amount");
        assert.equal(convertEvent.conversion_fee, 0, "unexpected conversion fee");

        assert.equal(toTokenPriceDataEvent.reserve_ratio, 0.5, "unexpected reserve_ratio");
        console.log(initialToTokenReserveBalance)
        console.log(convertEvent.return)
        const expectedToTokenReserveBalance = parseFloat(new BigNumber(initialToTokenReserveBalance).minus(convertEvent.return));
        assert.equal(parseFloat(toTokenPriceDataEvent.reserve_balance), expectedToTokenReserveBalance, "unexpected reserve_balance");

        assert.equal(fromTokenPriceDataEvent.reserve_ratio, 0.5, "unexpected reserve_ratio");
        const expectedFromTokenReserveBalance = parseFloat(new BigNumber(initialFromTokenReserveBalance).plus(amount));
        assert.equal(parseFloat(fromTokenPriceDataEvent.reserve_balance), expectedFromTokenReserveBalance, "unexpected reserve_balance");
    });

    it('2 hop convert', async function() {
        var minReturn = 0.100;
        const token = await _self.contract(tokenContract)

        let res = await token.transfer({ from: testUser1, to: networkContract, quantity: `1.00000000 ${tokenSymbol}`, memo: `1,${converter} ${networkTokenSymbol} ${converter2} ${tokenSymbol2},${minReturn},${testUser1}` }, _selfopts);
        let events = res.processed.action_traces[0].inline_traces[2].inline_traces[1].console.split("\n");

        let convertEvent = JSON.parse(events[0]);
        let priceDataEvent = JSON.parse(events[1]);
        assert.equal(convertEvent.return, 1.0000299998, "unexpected conversion result");
        assert.equal(convertEvent.conversion_fee, 0, "unexpected conversion fee");

        assert.equal(priceDataEvent.reserve_ratio, 0.5, "unexpected reserve_ratio");
        assert.equal(priceDataEvent.reserve_balance, 100000.9999700002, "unexpected reserve_balance");
        
        events = res.processed.action_traces[0].inline_traces[2].inline_traces[2].inline_traces[2].inline_traces[1].console.split("\n");
        convertEvent = JSON.parse(events[0]);
        priceDataEvent = JSON.parse(events[1]);
        assert.equal(convertEvent.return, 0.99802094, "unexpected conversion result");
        assert.equal(convertEvent.conversion_fee, 0.00099951, "unexpected conversion fee");

        assert.equal(priceDataEvent.reserve_ratio, 0.5, "unexpected reserve_ratio");
        assert.equal(priceDataEvent.reserve_balance, 99999.00197906, "unexpected reserve_balance");
    });


    it("verifies it's not possible to do a conversion with a destination wallet that's different than the origin account", async () => {
        const bntToken = await getEos(testUser1).contract(networkToken);
        const minReturn = '0.0000000001';
        const thirdPartyAccount = 'eosio';

        const conversion = bntToken.transfer(
            {
                from: testUser1,
                to: networkContract,
                quantity: `5.0000000000 ${networkTokenSymbol}`,
                memo: `1,${bntConverter} SYS,${minReturn},${thirdPartyAccount}`
            },
            { authorization: `${testUser1}@active` }
        );

        await ensureContractAssertionError(conversion, ERRORS.INVALID_TARGET_ACCOUNT);
    });

    it("verifies an error is thrown when trying to use a non-converter account name as part of the path", async () => {
        const bntToken = await getEos(testUser1).contract(networkToken);
        const minReturn = '0.0000000001';

        const conversion = bntToken.transfer(
            {
                from: testUser1,
                to: networkContract,
                quantity: `5.0000000000 ${networkTokenSymbol}`,
                memo: `1,${testUser2} ${networkTokenSymbol},${minReturn},${testUser1}`
            },
            { authorization: `${testUser1}@active` }
        );

        await ensureContractAssertionError(conversion, ERRORS.CONVERTER_DOESNT_EXIST);
    });
});
