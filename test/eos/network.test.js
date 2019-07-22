
const chai = require('chai');
var assert = chai.assert;

const {  
    expectError, 
    expectNoError,
} = require('./common/utils')

const { 
    getBalance,
    convertBNT,
    convertTwice,
} = require('./common/token')

const { ERRORS } = require('./common/errors')
const BigNumber = require('bignumber.js');
const user = 'rick'

describe('Test: BancorNetwork', () => {
    it('test 1 hop conversion - sell bnt to buy eos', async function () {
        const amount = 2.00000000;
        const convertReturn = 1.99960008; //expectation 
        const amountStr = '2.00000000';

        //relay starting balance of bnt
        var result = await getBalance('bnt2eoscnvrt','bntbntbntbnt','BNT')
        assert.equal(result.rows.length, 1)    
        const initialReserveBalanceFrom = parseFloat(result.rows[0].balance.split(' ')[0])
        //relay starting balance of eos
        result = await getBalance('bnt2eoscnvrt','eosio.token','EOS')
        assert.equal(result.rows.length, 1)    
        const initialReserveBalanceTo = parseFloat(result.rows[0].balance.split(' ')[0])
        
        //user starting balance of bnt
        result = await getBalance('rick','bntbntbntbnt','BNT')
        assert.equal(result.rows.length, 1)    
        const initialUserBalanceFrom = parseFloat(result.rows[0].balance.split(' ')[0])
        //user starting balance of bnteos
        result = await getBalance('rick','eosio.token','EOS')
        assert.equal(result.rows.length, 1) 
        const initialUserBalanceTo = parseFloat(result.rows[0].balance.split(' ')[0])
        
        //perform the conversion
        const res = await expectNoError(convertBNT(amountStr, 'EOS'));
        const events = res.processed.action_traces[0].inline_traces[2].inline_traces[1].console.split("\n");

        const convertEvent = JSON.parse(events[0]);
        const fromTokenPriceDataEvent = JSON.parse(events[1]);
        const toTokenPriceDataEvent = JSON.parse(events[2]);
        //console.log(events) TODO

        //expectation 
        const expectedReserveBalanceFrom = initialReserveBalanceFrom + amount;
        const expectedReserveBalanceTo = initialReserveBalanceTo - convertReturn;

        result = await getBalance('bnt2eoscnvrt','bntbntbntbnt','BNT')
        const currentReserveBalanceFrom = parseFloat(result.rows[0].balance.split(' ')[0])
        result = await getBalance('bnt2eoscnvrt','eosio.token','EOS')
        const currentReserveBalanceTo = parseFloat(result.rows[0].balance.split(' ')[0])
        
        result = initialReserveBalanceTo - currentReserveBalanceTo
        
        const tolerance = 0.00035992
        const deltaReserveFrom = Math.abs(currentReserveBalanceFrom - expectedReserveBalanceFrom)
        const deltaReserveTo = Math.abs(currentReserveBalanceTo - expectedReserveBalanceTo)
        const deltaReturn = Math.abs(convertReturn - result)

        //instead of using equal we use greater/equal due to imprecision from doubles in the formula
        assert.isAtMost(deltaReserveFrom, tolerance, "unexpected reserve_balance - bnt");
        assert.isAtMost(deltaReserveTo, tolerance, "unexpected reserve_balance - eos")
        assert.isAtMost(deltaReturn, tolerance, "unexpected return - eos");
                
        const expectedUserBalanceFrom = initialUserBalanceFrom - amount;
        const expectedUserBalanceTo = initialUserBalanceTo - convertReturn;

        result = await getBalance('rick','bntbntbntbnt','BNT')
        const currentUserBalanceFrom = parseFloat(result.rows[0].balance.split(' ')[0])
        result = await getBalance('rick','bnt2eosrelay','BNTEOS')
        const currentUserBalanceTo = parseFloat(result.rows[0].balance.split(' ')[0])

        assert.isAtMost(currentUserBalanceFrom, expectedUserBalanceFrom, "unexpected user_balance - bnt");
        assert.isAtLeast(currentUserBalanceTo, expectedUserBalanceTo, "unexpected user_balance - eos");
    })
    it('test 2 hop conversion - sell eos to buy bnt, then sell bnt to buy sys', async function() {
        const amount = 1.00000000; //amount of EOS to be sold for BNT
        const bntReturn = 1.00029999; //amount of BNT to be sold for SYS
        const sysReturn = 0.98029670; //amount of SYS bought 
        const totalFee = 0.01990324 //sysReturn if there was 0% fee (1.00019994) - actual sysReturn 
        const tolerance = 0.00035992 //maximum discrepancy, same as before
        const amountStr = '1.0000'; 
        
        // get user's initial EOS balance
        result = await getBalance('rick', 'eosio.token', 'EOS');
        assert.equal(result.rows.length, 1)    
        const initialUserBalanceFrom = parseFloat(result.rows[0].balance.split(' ')[0])
        // get BNTSYS relay's initial BNT balance
        var result = await getBalance('bnt2syscnvrt','bntbntbntbnt', 'BNT');
        assert.equal(result.rows.length, 1)    
        const initialReserveBalanceFrom = parseFloat(result.rows[0].balance.split(' ')[0])
        // get user's initial SYS balance
        result = await getBalance('rick', 'fakeos', 'SYS');
        assert.equal(result.rows.length, 1)    
        const initialUserBalanceTo = parseFloat(result.rows[0].balance.split(' ')[0])
        // get BNTSYS relay's initial SYS balance
        var result = await getBalance('bnt2syscnvrt', 'fakeos', 'SYS');
        assert.equal(result.rows.length, 1)    
        const initialReserveBalanceTo = parseFloat(result.rows[0].balance.split(' ')[0])

        // perform conversion
        const res = await expectNoError(convertTwice(amountStr, 'eosio.token', 'EOS', 'SYS'));
        const events = res.processed.action_traces[0].inline_traces[2].inline_traces[1].console.split("\n");

        const convertEvent = JSON.parse(events[0]);
        const fromTokenPriceDataEvent = JSON.parse(events[1]);
        const toTokenPriceDataEvent = JSON.parse(events[2]);
        //console.log(events) TODO

        const expectedUserBalanceFrom = initialUserBalanceFrom - amount //EOS - user
        const expectedUserBalanceTo = initialUserBalanceTo + sysReturn //SYS - user
        const expectedReserveBalanceFrom = initialReserveBalanceFrom + bntReturn //BNT - relay
        const expectedReserveBalanceTo = initialReserveBalanceTo - sysReturn //SYS - relay

        // get user's current EOS balance
        result = await getBalance('rick', 'eosio.token', 'EOS');
        assert.equal(result.rows.length, 1)    
        const currentUserBalanceFrom = parseFloat(result.rows[0].balance.split(' ')[0])
        // get BNTSYS relay's current BNT balance
        var result = await getBalance('bnt2syscnvrt','bntbntbntbnt', 'BNT');
        assert.equal(result.rows.length, 1)    
        const currentReserveBalanceFrom = parseFloat(result.rows[0].balance.split(' ')[0])
        // get user's current SYS balance
        result = await getBalance('rick', 'fakeos', 'SYS');
        assert.equal(result.rows.length, 1)    
        const currentUserBalanceTo = parseFloat(result.rows[0].balance.split(' ')[0])
        // get BNTSYS relay's current SYS balance
        var result = await getBalance('bnt2syscnvrt', 'fakeos', 'SYS');
        assert.equal(result.rows.length, 1)    
        const currentReserveBalanceTo = parseFloat(result.rows[0].balance.split(' ')[0])

        const deltaUserBalanceFrom = Math.abs(currentUserBalanceFrom - expectedUserBalanceFrom)
        const deltaUserBalanceTo = Math.abs(currentUserBalanceTo - expectedUserBalanceTo)
        const deltaReserveBalanceFrom = Math.abs(currentReserveBalanceFrom - expectedReserveBalanceFrom)
        const deltaReserveBalanceTo = Math.abs(currentReserveBalanceTo - expectedReserveBalanceTo)

        // check that relay's new SYS balance decreased appropriately
        assert.isAtMost(deltaReserveBalanceTo, tolerance, "fakeos sys invalid");
        // check that SYS relay's new BNT balance increased appropriately
        assert.isAtMost(deltaReserveBalanceFrom, tolerance, "fakeos bnt inavlid");
        // check that user's EOS balance decreased appropriately
        assert.isAtMost(deltaUserBalanceFrom, tolerance, "user's eos invalid");
        // check that user's SYS balance increased appropriately
        assert.isAtMost(deltaUserBalanceTo, tolerance, "user's sys invalid");
    })
    it("verifies throw error with a destination wallet different than the origin account", async () => {
        expectError(
            convertBNT('5.00000000', 'BNT', undefined, user, 'eosio'), 
            ERRORS.INVALID_TARGET_ACCOUNT
        );
    });
    it("verifies thrown error when using a non-converter account name as part of the path", async () => {
        expectError(
            convertBNT('5.00000000', 'BNT', user), 
            ERRORS.CONVERTER_DOESNT_EXIST
        );
    })
    it("verifies thrown error when exceeding user's available balance", async () => {
        expectError(
            convertBNT('500.00000000'), 
            ERRORS.OVER_SPENDING
        );
    })
    it("verifies thrown error when using a negative balance", async () => {
        expectError(
            convertBNT('-5.00000000'), 
            ERRORS.POSITIVE_TRANSFER
        );
    })
});    