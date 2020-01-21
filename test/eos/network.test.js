const assert = require('chai').assert
const Decimal = require('decimal.js')
const config = require('../../config/accountNames.json')
const {  
    expectError, 
    expectNoError,
    randomAmount,
    createAccountOnChain,
    deductFee,
    extractEvents,
    toFixedRoundUp
} = require('./common/utils')

const {
    getBalance,
    convertBNT,
    convertTwice,
    convertMulti,
    convert
} = require('./common/token')

const { 
    getReserve
} = require('./common/converter')

const { ERRORS } = require('./common/errors')
const user1 = config.MASTER_ACCOUNT
const user2 = config.TEST_ACCOUNT
const multiConverter = config.MULTI_CONVERTER_ACCOUNT
const bntToken = config.BNT_TOKEN_ACCOUNT

describe('Test: BancorNetwork', () => {
    describe('Affiliate Fees', async () => {
        it("should throw when passing affiliate account and inappropriate fee", async () => {
            await expectError(
                convertMulti('1.0000', 'BNTEOS', 'BNT', multiConverter, user1, '0.00000001', user2, 0),
                "inappropriate affiliate fee"
            )
            await expectError(
                convertMulti('1.0000', 'BNTEOS', 'BNT', multiConverter, user1, '0.00000001', user2, -10),
               "inappropriate affiliate fee"
            )
            await expectError(
                convertMulti('1.0000', 'BNTEOS', 'BNT', multiConverter, user1, '0.00000001', user2, 9000000),
                "inappropriate affiliate fee"
            )
        })
        it("should throw when passing affiliate account that is not actually an account", async () => {
            await expectError(
                convertMulti('1.0000', 'BNTEOS', 'BNT', multiConverter, user1, '0.00000001', "notauser", 3000),
                "affiliate is not an account"
            )
        })
        it("should throw when affiliate fee would bring conversion below min return", async () => {
            await expectError(
                convertMulti('1.0000', 'BNTEOS', 'BNT', multiConverter, user1, '0.9900000', user2, 29000),
                "below min return"
            )
        })
        it("1 Hop with Affiliate Fee, SingleConverter", async () => {
            const fee = 29000;
            let result = await getBalance(user1, bntToken, 'BNT')
            const user1beforeBNT = result.rows[0].balance.split(' ')[0]

            result = await getBalance(user2, bntToken, 'BNT')
            const user2beforeBNT = result.rows[0].balance.split(' ')[0]

            const { affiliate: [affiliateEvent] } = await extractEvents(
                convertMulti('1.0000', 'BNTEOS', 'BNT', multiConverter, user1, '0.00000001', user2, fee)
            )
            const returnedAmountEvent = affiliateEvent.return.split(' ')[0]
            const feeAmountEvent = affiliateEvent.affiliate_fee.split(' ')[0]
            const expectedReturnAfterFee = toFixedRoundUp(deductFee(returnedAmountEvent, fee, 1), 8)

            const feeAmountPaid = Decimal(returnedAmountEvent).sub(expectedReturnAfterFee).toFixed(8);
            result = await getBalance(user1, bntToken, 'BNT')
            const user1afterBNT = result.rows[0].balance.split(' ')[0]

            result = await getBalance(user2, bntToken, 'BNT')
            const user2afterBNT = result.rows[0].balance.split(' ')[0]

            const deltaUser1BNT = Decimal(user1afterBNT).sub(user1beforeBNT).toFixed(8)
            const deltaUser2BNT = Decimal(user2afterBNT).sub(user2beforeBNT).toFixed(8)

            assert.equal(deltaUser1BNT, expectedReturnAfterFee, 'unexpected return on conversion')
            assert.equal(deltaUser2BNT, feeAmountPaid, 'unexpected affiliate fee paid')
            assert.equal(feeAmountEvent, feeAmountPaid, 'unexpected affiliate fee event')
        })
        it("Selling BNT is a false positive, should not trigger affiliate fee", async function() {
            let result = await getBalance(user1, 'eosio.token', 'EOS')
            const user1beforeEOS = result.rows[0].balance.split(' ')[0]

            result = await getBalance(user2, bntToken, 'BNT')
            const user2beforeBNT = result.rows[ 0].balance.split(' ')[0]

            const events = await extractEvents(
                convertBNT('1.00000000', 'EOS', undefined, user1, user1, user2, 29000),
            )

            const expectedReturn = parseFloat(events.conversion[0].return);

            result = await getBalance(user1, 'eosio.token', 'EOS')
            const user1afterEOS = result.rows[0].balance.split(' ')[0]

            result = await getBalance(user2, bntToken, 'BNT')
            const user2afterBNT = result.rows[0].balance.split(' ')[0]

            const deltaUser1EOS = parseFloat(user1afterEOS) - parseFloat(user1beforeEOS)
            const deltaUser2BNT = parseFloat(user2afterBNT) - parseFloat(user2beforeBNT)

            assert.equal(deltaUser1EOS.toFixed(4), expectedReturn.toFixed(4), 'unexpected return on conversion')
            assert.equal(deltaUser2BNT, 0, 'unexpected affiliate fee paid')
        })
    })
    it('verifies that converting reserve --> reserve return == converting reserve --> relay & relay --> reserve (2 different transactions)', async function() {
        const initialBNTBalance = (await getBalance(user1, bntToken, 'BNT')).rows[0].balance.split(' ')[0];
        
        let conversionEvent;
        let returnedAmount;
        conversionEvent = (await extractEvents(
            convert(`${randomAmount({min: 1, max: 10, decimals: 8 })} BNT`, bntToken, ['bnt2bbbcnvrt', 'BBB'])
        )).conversion[0]
        returnedAmount = Decimal(conversionEvent.return).toFixed(8)

        conversionEvent = (await extractEvents(
            convert(`${returnedAmount} BBB`, 'bbb', ['bnt2bbbcnvrt', 'BNTBBB'])
        )).conversion[0]
        returnedAmount = Decimal(conversionEvent.return).toFixed(8)

        conversionEvent = (await extractEvents(
            convert(`${returnedAmount} BNTBBB`, 'bnt2bbbrelay', ['bnt2bbbcnvrt', 'BNT'])
        )).conversion[0]

        const finalBNTBalance = (await getBalance(user1, bntToken, 'BNT')).rows[0].balance.split(' ')[0];
        
        const tolerance = 0.00000003;
        const balancesDelta = Math.abs(Decimal(finalBNTBalance).sub(initialBNTBalance))
        assert.isAtMost(balancesDelta, tolerance, 'balanced should be equal');
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
            convert('1.00000000 BNT', bntToken, [`${multiConverter}:BNTEOS`, 'EOS'], user1, accountWithNoBalanceEntry),
            ERRORS.MUST_HAVE_TOKEN_ENTRY
        )
    })
})
