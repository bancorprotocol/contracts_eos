require("babel-core/register");
require("babel-polyfill");
import Eos from 'eosjs';
import { assert } from 'chai';
import 'mocha';
import { ensureContractAssertionError, getEos, getBalance, getSupply } from './utils';
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
    const tokenContract = 'aa';
    const tokenSymbol = "TKNA";

    const converter2 = 'cnvtbb';
    const networkContract = 'thisisbancor';
    const bntConverter = 'bnt2eoscnvrt';
    const networkTokenSymbol = "BNT";
    const networkToken = 'bnt';
    const tokenSymbol2 = "TKNB";
    const testUser1 = 'test1';
    const testUser2 = 'test2';
    const tokenContract2 = 'bb';
    const keyFile = JSON.parse(fs.readFileSync(path.resolve(process.env.ACCOUNTS_PATH, `${testUser1}.json`)).toString());
    const codekey = keyFile.privateKey;
    const _self = Eos({ httpEndpoint:host(), keyProvider:codekey });
    const _selfopts = { authorization:[`${testUser1}@active`] };
    
    it('simple convert', async function() {
        const minReturn = 0.100;
        const amount = '2.0000000000'
        
        const initialFromTokenReserveBalance = await getBalance(networkToken, converter);
        const initialToTokenReserveBalance = await getBalance(tokenContract, converter);
    
        const token = await _self.contract(networkToken)
        const res = await token.transfer({ from: testUser1, to: networkContract, quantity: `${amount} ${networkTokenSymbol}`, memo: `1,${converter} ${tokenSymbol},${minReturn},${testUser1}` }, _selfopts);
        const events = res.processed.action_traces[0].inline_traces[2].inline_traces[1].console.split("\n");

        const convertEvent = JSON.parse(events[0]);
        const fromTokenPriceDataEvent = JSON.parse(events[1]);
        const toTokenPriceDataEvent = JSON.parse(events[2]);

        assert.equal(convertEvent.return, 1.99996000, "unexpected conversion return amount");
        assert.equal(convertEvent.conversion_fee, 0, "unexpected conversion fee");
        assert.equal(toTokenPriceDataEvent.reserve_ratio, 0.5, "unexpected reserve_ratio");

        const expectedToTokenReserveBalance = parseFloat(new BigNumber(initialToTokenReserveBalance).minus(convertEvent.return));
        assert.equal(parseFloat(toTokenPriceDataEvent.reserve_balance), expectedToTokenReserveBalance, "unexpected reserve_balance");

        assert.equal(fromTokenPriceDataEvent.reserve_ratio, 0.5, "unexpected reserve_ratio");
        
        const expectedFromTokenReserveBalance = parseFloat(new BigNumber(initialFromTokenReserveBalance).plus(amount));
        assert.equal(parseFloat(fromTokenPriceDataEvent.reserve_balance), expectedFromTokenReserveBalance, "unexpected reserve_balance");
    
        const expectedSmartSupply = await getSupply('tknbntaa', 'BNTTKNA');
        assert.equal(parseFloat(expectedSmartSupply).toFixed(9), parseFloat(toTokenPriceDataEvent.smart_supply).toFixed(9), 'unexpected smart supply');
    });

    it('2 hop convert', async function() {
        const minReturn = 0.100;
        const amount = '1.00000000'
        const token = await _self.contract(tokenContract)
        const initialToTokenReserveBalance = await getBalance(tokenContract2, converter2);
        const initialTestUserTokenBBalance = await getBalance(tokenContract2, testUser1);

        const res = await token.transfer({ from: testUser1, to: networkContract, quantity: `${amount} ${tokenSymbol}`, memo: `1,${converter} ${networkTokenSymbol} ${converter2} ${tokenSymbol2},${minReturn},${testUser1}` }, _selfopts);
        let events = res.processed.action_traces[0].inline_traces[2].inline_traces[1].console.split("\n");

        let convertEvent = JSON.parse(events[0]);
        let priceDataEvent = JSON.parse(events[2]);
        assert.equal(convertEvent.conversion_fee, 0, "unexpected conversion fee");

        assert.equal(priceDataEvent.reserve_ratio, 0.5, "unexpected reserve_ratio");
        
        events = res.processed.action_traces[0].inline_traces[2].inline_traces[2].inline_traces[2].inline_traces[1].console.split("\n");
        convertEvent = JSON.parse(events[0]);
        priceDataEvent = JSON.parse(events[2]);
        const currentTestUserTokenBBalance = await getBalance(tokenContract2, testUser1);
        const expectedReturn = parseFloat(new BigNumber(currentTestUserTokenBBalance).minus(initialTestUserTokenBBalance));
        assert(Math.abs(parseFloat(convertEvent.return) - expectedReturn) <= 0.000001 , "unexpected conversion result"); // TODO: Remove this once we change formulas to be uint based, and precision issues are gone

        const expectedSmartSupply = await getSupply('tknbntbb', 'BNTTKNB');
        
        assert.equal(expectedSmartSupply, parseFloat(priceDataEvent.smart_supply), 'unexpected smart supply');
        const expectedConversionFee = (parseFloat(convertEvent.conversion_fee) / (parseFloat(convertEvent.conversion_fee) + parseFloat(convertEvent.return))).toFixed(5);
        assert.equal(expectedConversionFee, 0.002, "unexpected conversion fee");
        
        assert.equal(priceDataEvent.reserve_ratio, 0.5, "unexpected reserve_ratio");

        const expectedFromTokenReserveBalance = parseFloat(new BigNumber(initialToTokenReserveBalance).minus(convertEvent.return));
        assert(Math.abs(parseFloat(priceDataEvent.reserve_balance) - expectedFromTokenReserveBalance) <= 0.000001, "unexpected reserve_balance");
    });

    it('[PriceDataEvents: reserve --> smart] 1 hop conversion', async function() {
        const minReturn = '0.00000001';
        const amount = '1.0000000000'
        const RelaySymbol = `BNT${tokenSymbol}`;
        
        const initialFromTokenReserveBalance = await getBalance(networkToken, converter);
    
        const token = await _self.contract(networkToken)
        const res = await token.transfer({ from: testUser1, to: networkContract, quantity: `${amount} ${networkTokenSymbol}`, memo: `1,${converter} ${RelaySymbol},${minReturn},${testUser1}` }, _selfopts);
        const events = res.processed.action_traces[0].inline_traces[2].inline_traces[1].console.split("\n");

        const fromTokenPriceDataEvent = JSON.parse(events[1]);

        assert.equal(fromTokenPriceDataEvent.reserve_ratio, 0.5, "unexpected reserve_ratio");
        
        const expectedFromTokenReserveBalance = parseFloat(new BigNumber(initialFromTokenReserveBalance).plus(amount));
        assert.equal(parseFloat(fromTokenPriceDataEvent.reserve_balance), expectedFromTokenReserveBalance, "unexpected reserve_balance");
    
        const expectedSmartSupply = await getSupply('tknbntaa', RelaySymbol);
        assert.equal(parseFloat(expectedSmartSupply).toFixed(9), parseFloat(fromTokenPriceDataEvent.smart_supply).toFixed(9), 'unexpected smart supply');
    });

    it('[PriceDataEvents: smart --> reserve] 1 hop conversion', async function() {
        const minReturn = '0.0000000001';
        const amount = '0.0100000000';
        const RelaySymbol = `BNT${tokenSymbol}`;
        
        const initialFromTokenReserveBalance = await getBalance(networkToken, converter);
    
        const token = await _self.contract('tknbntaa')
        const res = await token.transfer({ from: testUser1, to: networkContract, quantity: `${amount} ${RelaySymbol}`, memo: `1,${converter} ${networkTokenSymbol},${minReturn},${testUser1}` }, _selfopts);
        const events = res.processed.action_traces[0].inline_traces[2].inline_traces[1].console.split("\n");

        const convertEvent = JSON.parse(events[0]);
        const fromTokenPriceDataEvent = JSON.parse(events[1]);
        assert.equal(fromTokenPriceDataEvent.reserve_ratio, 0.5, "unexpected reserve_ratio");
        
        const expectedFromTokenReserveBalance = parseFloat(new BigNumber(initialFromTokenReserveBalance).minus(convertEvent.return));

        assert.equal(parseFloat(fromTokenPriceDataEvent.reserve_balance), expectedFromTokenReserveBalance, "unexpected reserve_balance");
    
        const expectedSmartSupply = await getSupply('tknbntaa', RelaySymbol);
        assert.equal(parseFloat(expectedSmartSupply).toFixed(9), parseFloat(fromTokenPriceDataEvent.smart_supply).toFixed(9), 'unexpected smart supply');
    });

    it.skip('verifies that converting reserve --> reserve returns the same amount (after fees are deducted) as converting reserve --> relay & relay --> reserve (2 different transactions)', async function() {
        const minReturn = '0.0000000001';
        const amount = '5.0000000000';
        
        const bntTokenUser1 = await getEos(testUser1).contract(networkToken);

        const bntTokenUser2 = await getEos(testUser2).contract(networkToken);
        const tokenDRelay = await getEos(testUser2).contract('tknbntdd');
        
        await bntTokenUser1.transfer(
            { from: testUser1, to: networkContract, quantity: `${amount} ${networkTokenSymbol}`, memo: `1,cnvtee TKNE,${minReturn},${testUser1}` }, 
            { authorization: `${testUser1}@active` }
        );
        
        await bntTokenUser2.transfer(
            { from: testUser2, to: networkContract, quantity: `${amount} ${networkTokenSymbol}`, memo: `1,cnvtdd BNTTKND,${minReturn},${testUser2}` }, 
            { authorization: `${testUser2}@active` }
        );
        
        const relayBalance = await getBalance('tknbntdd', testUser2);
        
        await tokenDRelay.transfer(
            { from: testUser2, to: networkContract, quantity: `${relayBalance} BNTTKND`, memo: `1,cnvtdd TKND,${minReturn},${testUser2}` }, 
            { authorization: `${testUser2}@active` }
        );
        
        const tokenBalanceUser1 = await getBalance('ee', testUser1);
        const tokenBalanceUser2 = await getBalance('dd', testUser2);

        assert.equal(tokenBalanceUser1, tokenBalanceUser2, 'balanced should be equal');
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
