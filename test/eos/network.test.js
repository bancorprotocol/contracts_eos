const assert = require('chai').assert

const config = require('../../config/accountNames.json')
const {  
    expectError, 
    expectNoError,
    createAccountOnChain
} = require('./common/utils')

const {
    getBalance,
    convertBNT,
    convertRelay,
    convertTwice
} = require('./common/token')

const { 
    getReserve
} = require('./common/converter')

const { ERRORS } = require('./common/errors')
const user1 = config.MASTER_ACCOUNT
const user2 = config.TEST_ACCOUNT
const multiConverter = config.MULTI_CONVERTER_ACCOUNT

describe('Test: BancorNetwork', () => {
    it.skip('verifies that converting reserve --> reserve return == converting reserve --> relay & relay --> reserve (2 different transactions)', async function() {
        const amount = '5.00000000';

        const tolerance =  0.00002;

        let res = await getBalance(user2,'bnt2bbbrelay','BNTBBB');
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
    it("ensures it's not possible to abuse RAM by planting a non-converter account as part of the conversion path", async () => {
        const fakeConverter = (await createAccountOnChain()).accountName;
        await expectError(
            convertTwice('1.0000', 'eosio.token', 'EOS', 'FAKETKN', multiConverter, fakeConverter), 
            ERRORS.MUST_HAVE_TOKEN_ENTRY
        )
    })
    it("ensures an error is thrown when a conversion destination account has no token balance entry", async () => {
        const accountWithNoBalanceEntry = (await createAccountOnChain()).accountName;
        await expectError(
            convertBNT('1.00000000', 'EOS', multiConverter, user1, accountWithNoBalanceEntry), 
            ERRORS.MUST_HAVE_TOKEN_ENTRY
        )
    })
})
