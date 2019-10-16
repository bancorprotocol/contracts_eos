
const chai = require('chai')
var assert = chai.assert

const {  
    expectError, 
    expectNoError,
} = require('./common/utils')

const { 
    get,
    getBalance,
    convertBNT,
    convertRelay,
    convertTwice,
} = require('./common/token')

const { ERRORS } = require('./common/errors')
const user1 = 'bnttestuser1'
const user2 = 'bnttestuser2'

describe('Test: BancorNetwork', () => {
    it('test 1 hop conversion - sell bnt to buy eos', async function () {
        const amount = 2.00000000
        const tolerance =  0.000001
        const convertReturn = 1.9996 //expectation 
        const amountStr = '2.00000000'

        //relay starting balance of bnt
        var result = await getBalance('bnt2eoscnvrt','bntbntbntbnt','BNT')
        assert.equal(result.rows.length, 1)    
        const initialReserveBalanceFrom = parseFloat(result.rows[0].balance.split(' ')[0])
        //relay starting balance of eos
        result = await getBalance('bnt2eoscnvrt','eosio.token','EOS')
        assert.equal(result.rows.length, 1)    
        const initialReserveBalanceTo = parseFloat(result.rows[0].balance.split(' ')[0])
        
        //user1 starting balance of bnt
        result = await getBalance(user1,'bntbntbntbnt','BNT')
        assert.equal(result.rows.length, 1)    
        const initialUserBalanceFrom = parseFloat(result.rows[0].balance.split(' ')[0])
        //user1 starting balance of eos
        result = await getBalance(user1,'eosio.token','EOS')
        assert.equal(result.rows.length, 1) 
        const initialUserBalanceTo = parseFloat(result.rows[0].balance.split(' ')[0])
        
        //perform the conversion
        const res = await expectNoError(convertBNT(amountStr, 'EOS'))
        const events = res.processed.action_traces[0].inline_traces[2].inline_traces[1].console.split("\n")

        const convertEvent = JSON.parse(events[0])
        const fromTokenPriceDataEvent = JSON.parse(events[1])
        const toTokenPriceDataEvent = JSON.parse(events[2])
        
        assert.equal(convertEvent.conversion_fee, 0, "unexpected conversion fee, from event")
        assert.equal(toTokenPriceDataEvent.reserve_ratio, 0.5, "unexpected reserve ratio, from event toToken")
        assert.equal(fromTokenPriceDataEvent.reserve_ratio, 0.5, "unexpected reserve ratio, from event fromToken")
        
        //expectation 
        const expectedReserveBalanceFrom = initialReserveBalanceFrom + amount
        const expectedReserveBalanceTo = initialReserveBalanceTo - convertReturn

        result = await getBalance('bnt2eoscnvrt','bntbntbntbnt','BNT')
        const currentReserveBalanceFrom = parseFloat(result.rows[0].balance.split(' ')[0])
        result = await getBalance('bnt2eoscnvrt','eosio.token','EOS')
        const currentReserveBalanceTo = parseFloat(result.rows[0].balance.split(' ')[0])
        
        result = initialReserveBalanceTo - currentReserveBalanceTo
        
        const deltaReserveFrom = Math.abs(currentReserveBalanceFrom - expectedReserveBalanceFrom)
        const deltaReserveTo = Math.abs(currentReserveBalanceTo - expectedReserveBalanceTo)
        const deltaReturnEvent = Math.abs(convertReturn - convertEvent.return)
        const deltaReturn = Math.abs(convertReturn - result)
        
        //instead of using equal we use greater/equal due to imprecision from doubles in the formula
        assert.isAtMost(deltaReserveFrom, tolerance, "unexpected bnt reserve_balance")
        assert.isAtMost(deltaReserveTo, tolerance, "unexpected eos reserve_balance")    
        assert.isAtMost(deltaReturnEvent, tolerance, "unexpected eos return - event")     
        assert.isAtMost(deltaReturn, tolerance, "unexpected eos return")        
                
        assert.equal(currentReserveBalanceFrom, parseFloat(fromTokenPriceDataEvent.reserve_balance), "unexpected reserveFROM - event")
        assert.equal(currentReserveBalanceTo, parseFloat(toTokenPriceDataEvent.reserve_balance), "unexpected reserveTO - event")

        const expectedUserBalanceFrom = initialUserBalanceFrom - amount
        const expectedUserBalanceTo = initialUserBalanceTo + convertReturn

        result = await getBalance(user1,'bntbntbntbnt','BNT')
        const currentUserBalanceFrom = parseFloat(result.rows[0].balance.split(' ')[0])
        result = await getBalance(user1,'eosio.token','EOS')
        const currentUserBalanceTo = parseFloat(result.rows[0].balance.split(' ')[0])

        assert.isAtMost(currentUserBalanceFrom, expectedUserBalanceFrom, "unexpected user_balance - bnt")
        assert.isAtLeast(currentUserBalanceTo, expectedUserBalanceTo, "unexpected user_balance - eos")

        result = await get('bnt2eosrelay', 'BNTEOS')
        const expectedSmartSupply = parseFloat(result.rows[0].supply.split(' ')[0])
        assert.equal(expectedSmartSupply, parseFloat(toTokenPriceDataEvent.smart_supply), 'unexpected smart supply')
    })
    it('test 2 hop conversion - sell eos to buy bnt, then sell bnt to buy sys', async function() {
        const amount = 1.00000000 //amount of EOS to be sold for BNT
        const bntReturn = 1.00029999 //amount of BNT to be sold for SYS
        const sysReturn = 0.98029670 //amount of SYS bought 
        const totalFee = 0.01990324 //sysReturn if there was 0% fee (1.00019994) - actual sysReturn 
        const tolerance =  0.0003 //maximum discrepancy
        const amountStr = '1.0000' 
        
        // get user1's initial EOS balance
        result = await getBalance(user1, 'eosio.token', 'EOS')
        assert.equal(result.rows.length, 1)    
        const initialUserBalanceFrom = parseFloat(result.rows[0].balance.split(' ')[0])
        // get BNTSYS relay's initial BNT balance
        var result = await getBalance('bnt2syscnvrt','bntbntbntbnt', 'BNT')
        assert.equal(result.rows.length, 1)    
        const initialReserveBalanceFrom = parseFloat(result.rows[0].balance.split(' ')[0])
        // get user1's initial SYS balance
        result = await getBalance(user1, 'fakeos', 'SYS')
        assert.equal(result.rows.length, 1)    
        const initialUserBalanceTo = parseFloat(result.rows[0].balance.split(' ')[0])
        // get BNTSYS relay's initial SYS balance
        var result = await getBalance('bnt2syscnvrt', 'fakeos', 'SYS')
        assert.equal(result.rows.length, 1)    
        const initialReserveBalanceTo = parseFloat(result.rows[0].balance.split(' ')[0])

        // perform conversion
        const res = await expectNoError(convertTwice(amountStr, 'eosio.token', 'EOS', 'SYS'))
        
        var events = res.processed.action_traces[0].inline_traces[2].inline_traces[1].console.split("\n")
        var convertEvent = JSON.parse(events[0])
        var priceDataEvent = JSON.parse(events[2])
        var delta = Math.abs(parseFloat(convertEvent.return) - bntReturn)
        
        assert.equal(convertEvent.conversion_fee, 0, "unexpected conversion fee #1")
        assert.equal(priceDataEvent.reserve_ratio, 0.5, "unexpected reserve_ratio")
        assert.isAtMost(delta, tolerance, "bad return from event - bnt")

        events = res.processed.action_traces[0].inline_traces[2].inline_traces[2].inline_traces[2].inline_traces[1].console.split("\n")
        convertEvent = JSON.parse(events[0])
        priceDataEvent = JSON.parse(events[2])
        
        assert.equal(priceDataEvent.reserve_ratio, 0.5, "unexpected reserve_ratio")

        const expectedUserBalanceFrom = initialUserBalanceFrom - amount //EOS - user1
        const expectedUserBalanceTo = initialUserBalanceTo + sysReturn //SYS - user1

        const expectedReserveBalanceFrom = initialReserveBalanceFrom + bntReturn //BNT - relay
        const expectedReserveBalanceTo = initialReserveBalanceTo - sysReturn //SYS - relay

        // get user1's current EOS balance
        result = await getBalance(user1, 'eosio.token', 'EOS')
        assert.equal(result.rows.length, 1)    
        const currentUserBalanceFrom = parseFloat(result.rows[0].balance.split(' ')[0])
        // get BNTSYS relay's current BNT balance
        var result = await getBalance('bnt2syscnvrt','bntbntbntbnt', 'BNT')
        assert.equal(result.rows.length, 1)    
        const currentReserveBalanceFrom = parseFloat(result.rows[0].balance.split(' ')[0])
        // get user1's current SYS balance
        result = await getBalance(user1, 'fakeos', 'SYS')
        assert.equal(result.rows.length, 1)    
        const currentUserBalanceTo = parseFloat(result.rows[0].balance.split(' ')[0])
        // get BNTSYS relay's current SYS balance
        var result = await getBalance('bnt2syscnvrt', 'fakeos', 'SYS')
        assert.equal(result.rows.length, 1)    
        const currentReserveBalanceTo = parseFloat(result.rows[0].balance.split(' ')[0])

        const deltaUserBalanceFrom = Math.abs(currentUserBalanceFrom - expectedUserBalanceFrom)
        const deltaUserBalanceTo = Math.abs(currentUserBalanceTo - expectedUserBalanceTo)
        const deltaReserveBalanceFrom = Math.abs(currentReserveBalanceFrom - expectedReserveBalanceFrom)
        const deltaReserveBalanceTo = Math.abs(currentReserveBalanceTo - expectedReserveBalanceTo)
        
        const expectedReturn = initialReserveBalanceTo - currentReserveBalanceTo
        delta = Math.abs(parseFloat(convertEvent.return) - expectedReturn)
        const feedelta = Math.abs(parseFloat(convertEvent.conversion_fee).toFixed(8) - totalFee)
        
        assert.isAtMost(feedelta, tolerance, "unexpected conversion fee #2")
        assert.isAtMost(delta, tolerance, "unexpected return from event")

        // check that relay's new SYS balance decreased appropriately
        assert.isAtMost(deltaReserveBalanceTo, tolerance, "fakeos sys invalid")
        // check that SYS relay's new BNT balance increased appropriately
        assert.isAtMost(deltaReserveBalanceFrom, tolerance, "fakeos bnt inavlid")
        // check that user1's EOS balance decreased appropriately
        assert.isAtMost(deltaUserBalanceFrom, tolerance, "user1's eos invalid")
        // check that user1's SYS balance increased appropriately
        assert.isAtMost(deltaUserBalanceTo, tolerance, "user1's sys invalid")

        result = await get('bnt2sysrelay', 'BNTSYS')
        const expectedSmartSupply = parseFloat(result.rows[0].supply.split(' ')[0])
        assert.equal(expectedSmartSupply, parseFloat(priceDataEvent.smart_supply), 'unexpected smart supply')
    })
    ///
    it('[PriceDataEvents: reserve --> smart] 1 hop conversion', async function() {
        const amount = '1.00000000'
            
        var res = await getBalance('bnt2eoscnvrt','bntbntbntbnt','BNT');
        const initialFromTokenReserveBalance = parseFloat(res.rows[0].balance.split(' ')[0])

        res = await expectNoError(convertBNT(amount)) //to BNTEOS
    
        const events = res.processed.action_traces[0].inline_traces[2].inline_traces[1].console.split("\n");

        const fromTokenPriceDataEvent = JSON.parse(events[1]);

        assert.equal(fromTokenPriceDataEvent.reserve_ratio, 0.5, "unexpected reserve_ratio");
        
        const expectedFromTokenReserveBalance = parseFloat(amount) + initialFromTokenReserveBalance;
        assert.equal(parseFloat(fromTokenPriceDataEvent.reserve_balance), expectedFromTokenReserveBalance, "unexpected reserve_balance");
    
        res = await get('bnt2eosrelay', 'BNTEOS');
        const expectedSmartSupply = parseFloat(res.rows[0].supply.split(' ')[0])
        assert.equal(expectedSmartSupply, parseFloat(fromTokenPriceDataEvent.smart_supply).toFixed(8), 'unexpected smart supply');
    });
    it('[PriceDataEvents: smart --> reserve] 1 hop conversion (do it in reverse)', async function() {
        const amount = '0.01000000'
            
        var res = await getBalance('bnt2eoscnvrt','bntbntbntbnt','BNT');
        const initialFromTokenReserveBalance = parseFloat(res.rows[0].balance.split(' ')[0])

        res = await expectNoError(convertRelay(amount)) //to BNTEOS
    
        const events = res.processed.action_traces[0].inline_traces[2].inline_traces[1].console.split("\n");

        const convertEvent = JSON.parse(events[0]);
        const fromTokenPriceDataEvent = JSON.parse(events[1]);

        assert.equal(fromTokenPriceDataEvent.reserve_ratio, 0.5, "unexpected reserve_ratio");
        
        const expectedFromTokenReserveBalance = initialFromTokenReserveBalance - parseFloat(convertEvent.return);
        assert.equal(parseFloat(fromTokenPriceDataEvent.reserve_balance), expectedFromTokenReserveBalance.toFixed(8), "unexpected reserve_balance");
    
        res = await get('bnt2eosrelay', 'BNTEOS');
        const expectedSmartSupply = parseFloat(res.rows[0].supply.split(' ')[0])
        assert.equal(expectedSmartSupply, parseFloat(fromTokenPriceDataEvent.smart_supply).toFixed(8), 'unexpected smart supply');
    });
    it('verifies that converting reserve --> reserve return == converting reserve --> relay & relay --> reserve (2 different transactions)', async function() {
        const amount = '5.00000000';

        const tolerance =  0.00002;

        var res = await getBalance(user2,'bnt2bbbrelay','BNTBBB');
        const user2BNTBBBBalanceBefore = parseFloat(res.rows[0].balance.split(' ')[0])
        res = await expectNoError(
            convertBNT(amount, 'AAA', 'bnt2aaacnvrt', user1)
        )
        res = await expectNoError(
            convertBNT(amount, 'BNTBBB', 'bnt2bbbcnvrt', user2)
        )
        
        res = await getBalance(user2,'bnt2bbbrelay','BNTBBB');
        const user2BNTBBBBalanceAfter = parseFloat(res.rows[0].balance.split(' ')[0])
        const delta = user2BNTBBBBalanceAfter - user2BNTBBBBalanceBefore
        
        res = await expectNoError(
            convertRelay(delta.toFixed(8), user2, 'bnt2bbbcnvrt', 'bnt2bbbrelay', 'BNTBBB', 'BBB')
        )
        
        res = await getBalance(user1,'aaa','AAA');
        const user1AAABalance = parseFloat(res.rows[0].balance.split(' ')[0])
        
        res = await getBalance(user2,'bbb','BBB');
        const user2BBBBalance = parseFloat(res.rows[0].balance.split(' ')[0])

        const deltaUser = Math.abs(user1AAABalance - user2BBBBalance)
        assert.isAtMost(deltaUser, tolerance, 'balanced should be equal');
    });
    it("verifies thrown error when using a non-converter account name as part of the path", async () => {
        await expectError(
            convertBNT('2.00000000', 'EOS', user1), 
            ERRORS.CONVERTER_DOESNT_EXIST
        )
    })
    it("verifies thrown error when exceeding user1's available balance", async () => {
        await expectError(
            convertBNT('2000000.00000000'), 
            ERRORS.OVER_SPENDING
        )
    })
    it("verifies thrown error when using a negative balance", async () => {
        await expectError(
            convertBNT('-2.00000000'), 
            ERRORS.POSITIVE_TRANSFER
        )
    })
    it("verifies throw error with a destination wallet different than the origin account", async () => {
        await expectError(
            convertBNT('2.00000000', 'EOS', undefined, user1, user2)
        )
    })
})    