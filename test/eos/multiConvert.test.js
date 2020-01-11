
const Decimal = require('decimal.js')
const chai = require('chai')
const assert = chai.assert

const {
    expectError,
    expectNoError,
    randomAmount,
    extractEvents,
    calculatePurchaseReturn,
    calculateSaleReturn,
    calculateQuickConvertReturn
} = require('./common/utils')

const {
    get,
    transfer,
    getBalance,
    convertBNT,
    convertEOS,
    convertMulti,
    convertTwice
} = require('./common/token')

const {
    createConverter,
    delConverter,
    setMultitoken,
    getConverter,
    getAccount,
    setreserve,
    delreserve,
    getReserve,
    getSettings,
    setStaking,
    enableStake,
    updateOwner,
    updateFee,
    setMaxfee,
    withdraw,
    fund
} = require('./common/converter')

const { ERRORS } = require('./common/errors')

const user1 = 'bnttestuser1'
const user2 = 'bnttestuser2'
const bntToken = 'bntbntbntbnt'
const multiToken = 'multi4tokens'
const multiConverter = 'multiconvert'

describe('Test: multiConverter', () => {
    describe('Converters Balances Management', async function() {
        it('set up multiConverter', async function () {
            await expectNoError( 
                setMultitoken(multiConverter) 
            )
            result = await getSettings(multiConverter)
            assert.equal(result.rows.length, 1)
            await expectNoError(
                setStaking(multiConverter)
            )
            await expectNoError(
                setMaxfee(multiConverter, 30000)
            )
        })
        it('setup converters', async function() {
            const AinitSupply = '1000.0000'
            const Asymbol = 'TKNA'
            
            const BinitSupply = '1000.0000'
            const Bsymbol = 'TKNB'

            const CinitSupply = '99000.00000000'
            const Csymbol = 'BNTEOS'

            const DinitSupply = '99000.00000000'
            const Dsymbol = 'RELAY'

            const EinitSupply = '99000.00000000'
            const Esymbol = 'RELAYB'

            await expectNoError(
                createConverter(user1, Asymbol, AinitSupply) 
            )
            result = await getConverter(Asymbol)
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].fee, 0, "converter fee not set correctly - TKNA")

            await expectNoError(
                createConverter(user2, Bsymbol, BinitSupply) 
            )
            await expectNoError(
                createConverter(user1, Csymbol, CinitSupply) 
            )
            await expectNoError(
                createConverter(user1, Dsymbol, DinitSupply) 
            )
            await expectNoError(
                createConverter(user1, Esymbol, EinitSupply) 
            )
        })
        it('setup reserves', async function() {
            const Aratio = 100000
            const Bratio = 300000
            const Cratio = 500000
            const Dratio = 500000
            const Eratio = 500000

            await expectNoError(
                setreserve(true, bntToken, 'BNT', multiConverter, 'TKNA', user1, Aratio) 
            )

            await expectNoError(
                setreserve(true, bntToken, 'BNT', multiConverter, 'TKNB', user2, Bratio) 
            )

            await expectNoError(
                setreserve(true, bntToken, 'BNT', multiConverter, 'BNTEOS', user1, Cratio) 
            )
            await expectNoError(
                setreserve(false, 'eosio.token', 'EOS', multiConverter, 'BNTEOS', user1, Cratio) 
            )

            await expectNoError(
                setreserve(true, bntToken, 'BNT', multiConverter, 'RELAY', user1, Dratio) 
            )
            await expectNoError(
                setreserve(false, 'eosio.token', 'EOS', multiConverter, 'RELAY', user1, Dratio) 
            )

            await expectNoError(
                setreserve(true, bntToken, 'BNT', multiConverter, 'RELAYB', user1, Eratio) 
            )
            await expectNoError(
                setreserve(false, 'eosio.token', 'EOS', multiConverter, 'RELAYB', user1, Eratio) 
            )
        })
        it('fund reserves', async function () {        
            //just for opening accounts
            await expectNoError(
                transfer(multiToken, '0.0001 TKNA', user2, user1, "")
            )
            await expectNoError(
                transfer(multiToken, '0.0001 TKNB', user1, user2, "")
            )
            await expectNoError( 
                transfer(bntToken, '1000.00000000 BNT', multiConverter, user1, 'fund;TKNA') 
            )
            await expectNoError( 
                transfer(bntToken, '1000.00000000 BNT', multiConverter, user2, 'fund;TKNB') 
            )
            await expectNoError( 
                transfer(bntToken, '990.00000000 BNT', multiConverter, user1, 'fund;BNTEOS') 
            )
            await expectNoError( 
                transfer('eosio.token', '990.0000 EOS', multiConverter, user1, 'fund;BNTEOS') 
            )

            await expectNoError( 
                transfer(bntToken, '999.00000000 BNT', multiConverter, user1, 'fund;RELAY') 
            )
            await expectNoError( 
                transfer('eosio.token', '990.0000 EOS', multiConverter, user1, 'fund;RELAY') 
            )

            await expectNoError( 
                transfer(bntToken, '999.00000000 BNT', multiConverter, user1, 'fund;RELAYB') 
            )
            await expectNoError( 
                transfer('eosio.token', '990.0000 EOS', multiConverter, user1, 'fund;RELAYB') 
            )
            
            result = await getReserve('BNT', multiConverter, 'BNTEOS')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].balance, '990.00000000 BNT', "withdrawal invalid - BNTEOS")

            result = await getReserve('EOS', multiConverter, 'BNTEOS')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].balance, '990.0000 EOS', "funded amount invalid - BNTEOS")
        })
        it("enable staking", async () => {
            await expectNoError(
                enableStake(user1, 'BNTEOS')
            )
            result = await getConverter('BNTEOS')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].stake_enabled, true, "converter not stake_enabled - TKNA")
            await expectError(
                updateFee(user2, 'BNTEOS', 10),
                ERRORS.PERMISSIONS
            )
        })
        it('ensures that fund and withdraw are working properly (pre-launch)', async () => {
            bntReserveBefore = '990.00000000 BNT'
            var result = await await getBalance(user1, bntToken, 'BNT')
            var bntUserBefore = result.rows[0].balance.split(' ')[0]
            await expectNoError(
                transfer(bntToken, '10.00000000 BNT', multiConverter, user1, 'fund;BNTEOS'),
            )
            result = await getReserve('BNT', multiConverter, 'BNTEOS')
            var bntReserveAfter = result.rows[0].balance
            assert.equal(bntReserveBefore, bntReserveAfter, 'BNT wasnt supposed to change yet - BNTEOS')

            await expectError(
                withdraw(user1, '11.00000000 BNT', 'BNTEOS'),
                "insufficient balance"
            ) 
            await expectNoError(
                withdraw(user1, '9.00000000 BNT', 'BNTEOS'),
            ) 
            result = await getAccount(user1, 'BNTEOS', 'BNT')
            var balance = result.rows[0].quantity.split(' ')[0]
            assert.equal(balance, '1.00000000', 'BNT wasnt supposed to change yet - BNTEOS')

            await expectNoError(
                transfer(bntToken, '9.00000001 BNT', multiConverter, user1, 'fund;BNTEOS'),
            )
            result = await getAccount(user1, 'BNTEOS', 'BNT')
            balance = result.rows[0].quantity.split(' ')[0]
            assert.equal(balance, '10.00000001', 'BNT wasnt supposed to change yet - BNTEOS')

            result = await getBalance(user1, bntToken, 'BNT')
            var bntUserAfter = result.rows[0].balance.split(' ')[0]
            assert.equal((parseFloat(bntUserBefore) - parseFloat(bntUserAfter)).toFixed(8), '10.00000001', 'BNT balance didnt go down appropriately - BNTEOS')
           
            eosReserveBefore = '990.0000 EOS'
            await expectNoError( 
                transfer('eosio.token', '10.0001 EOS', multiConverter, user1, 'fund;BNTEOS') 
            )
            result = await getReserve('EOS', multiConverter, 'BNTEOS')
            var eosReserveAfter = result.rows[0].balance
            assert.equal(eosReserveBefore, eosReserveAfter, 'EOS wasnt supposed to change yet - BNTEOS')
        })
        it('ensures that fund and liquidate are working properly (post-launch)', async () => {
            result = await getBalance(user1, multiToken, 'BNTEOS')
            assert.equal(result.rows.length, 1)
            let userSmartTokenBalanceBefore = parseFloat(result.rows[0].balance)
            
            await expectError( // insufficient balance in account row
                fund(user1, '2000.0000 BNTEOS'),
                "insufficient balance"
            )
            await expectNoError(
                fund(user1, '1000.0000 BNTEOS')
            )

            eosTemp = await getAccount(user1, "BNTEOS", "EOS");
            bntTemp = await getAccount(user1, "BNTEOS", "BNT");
            assert.equal(eosTemp.rows.length, 0, "was expecting to have no bank balance")
            assert.equal(bntTemp.rows.length, 0, "was expecting to have no bank balance")

            await expectError( // last fund cleared the accounts row
                fund(user1, '10000.0000 BNTEOS'),
                "cannot withdraw non-existant deposit"
            )
            
            // Fetch users balance and confirm he was awarded the 10000 BNT he asked for
            result = await getBalance(user1, multiToken, 'BNTEOS')
            assert.equal(result.rows.length, 1)
            let userSmartTokenBalanceAfter = parseFloat(result.rows[0].balance)
            let delta = (userSmartTokenBalanceAfter - userSmartTokenBalanceBefore).toFixed(4)
            assert.equal(delta, '1000.0000', 'incorrect BNTEOS issued')
            
            // Get relay reserve - BNT
            result = await getReserve('BNT', multiConverter, 'BNTEOS')
            bntReserveAfter = result.rows[0].balance
            assert.equal(bntReserveAfter, '1000.00000001 BNT', 'BNT was supposed to change - BNTEOS')

            // Get Relay reserve - EOS
            result = await getReserve('EOS', multiConverter, 'BNTEOS')
            eosReserveAfter = result.rows[0].balance
            assert.equal(eosReserveAfter, '1000.0001 EOS', 'EOS was supposed to change - BNTEOS')
            
            result = await getAccount(user1, 'BNTEOS', 'BNT')
            assert.equal(result.rows.length, 0, "was meant to lose the bank balance")
            
            result = await getAccount(user1, 'BNTEOS', 'EOS')
            assert.equal(result.rows.length, 0, "was meant to lose the bank balance")
            

            // Ends
            var result = await getBalance(user1, bntToken, 'BNT')
            bntUserBefore = result.rows[0].balance.split(' ')[0]
            result = await getBalance(user1, 'eosio.token', 'EOS')
            let eosUserBefore = result.rows[0].balance.split(' ')[0]

            
            // Liquidate Begins
            
            result = await getBalance(user1, multiToken, 'BNTEOS')
            let userRelayBefore = result.rows[0].balance.split(' ')[0]

            await expectNoError( 
                transfer(multiToken, '1000.0000 BNTEOS', multiConverter, user1, 'liquidate')
            )
        
            result = await getBalance(user1, bntToken, 'BNT')
            bntUserAfter = result.rows[0].balance.split(' ')[0]
            assert.equal(10, bntUserAfter - bntUserBefore, 'user bnt balance after is incorrect - BNTEOS')
            
            result = await getBalance(user1, 'eosio.token', 'EOS')
            let eosUserAfter = result.rows[0].balance.split(' ')[0]
            assert.equal(10, eosUserAfter - eosUserBefore, result.rows[0].balance, 0, 'user eos balance after is incorrect - BNTEOS')

            result = await getReserve('BNT', multiConverter, 'BNTEOS')
            bntReserveAfter = result.rows[0].balance.split(' ')[0]
            assert.equal(bntReserveAfter, '990.00000001', 'BNT reserve after is incorrect - BNTEOS')

            result = await getReserve('EOS', multiConverter, 'BNTEOS')
            eosReserveAfter = result.rows[0].balance.split(' ')[0]
            assert.equal(eosReserveAfter, '990.0001', 'EOS reserve after is incorrect - BNTEOS')

            result = await getBalance(user1, multiToken, 'BNTEOS')
            let userRelayAfter = result.rows[0].balance.split(' ')[0]
            assert.equal((userRelayBefore - userRelayAfter).toFixed(4), '1000.0000', 'smart token balance after is incorrect - BNTEOS')
        })
        it('ensures that converters balances sum is equal to the multiConverter\'s total BNT balance [Load Test]', async function () {
            this.timeout(10000)
            const bntTxs = []
            const multiTxs = []
            for (let i = 0; i < 5; i++) {
                bntTxs.push(
                    transfer(bntToken, `${randomAmount({min: 8, max: 12, decimals: 8 })} BNT`, 'thisisbancor', user1, 
                                     `1,${multiConverter}:TKNA TKNA,0.0001,${user1}`
                            ),
                    transfer(bntToken, `${randomAmount({min: 8, max: 12, decimals: 8 })} BNT`, 'thisisbancor', user2, 
                                     `1,${multiConverter}:TKNB TKNB,0.0001,${user2}`
                            ),
                )
                multiTxs.push(
                    transfer(multiToken, `${randomAmount({min: 1, max: 8, decimals: 4 })} TKNA`, 'thisisbancor', user1, 
                                       `1,${multiConverter}:TKNA BNT,0.0000000001,${user1}`
                            ),
                    transfer(multiToken, `${randomAmount({min: 1, max: 8, decimals: 4 })} TKNB`, 'thisisbancor', user2, 
                                       `1,${multiConverter}:TKNB BNT,0.0000000001,${user2}`
                            ),
                    transfer(multiToken, `${randomAmount({min: 1, max: 8, decimals: 4 })} TKNA`, 'thisisbancor', user1, 
                                       `1,${multiConverter}:TKNA BNT ${multiConverter}:TKNB TKNB,0.0001,${user1}`
                            ),
                    transfer(multiToken, `${randomAmount({min: 1, max: 8, decimals: 4 })} TKNB`, 'thisisbancor', user2, 
                                       `1,${multiConverter}:TKNB BNT ${multiConverter}:TKNA TKNA,0.0001,${user2}`
                            )
                )
            }
            await Promise.all(bntTxs)
            await Promise.all(multiTxs)

            const TokenAReserveBalance = Number((await getReserve('BNT', multiConverter, 'TKNA')).rows[0].balance.split(' ')[0])
            const TokenBReserveBalance = Number((await getReserve('BNT', multiConverter, 'TKNB')).rows[0].balance.split(' ')[0])
            const BNTEOSReserveBalance = Number((await getReserve('BNT', multiConverter, 'BNTEOS')).rows[0].balance.split(' ')[0])
            const RELAYReserveBalance = Number((await getReserve('BNT', multiConverter, 'RELAY')).rows[0].balance.split(' ')[0])
            const RELAYBReserveBalance = Number((await getReserve('BNT', multiConverter, 'RELAYB')).rows[0].balance.split(' ')[0])
            
            const totalBntBalance = (await getBalance(multiConverter, bntToken, 'BNT')).rows[0].balance.split(' ')[0]
            assert.equal((TokenAReserveBalance + TokenBReserveBalance + BNTEOSReserveBalance + RELAYReserveBalance + RELAYBReserveBalance).toFixed(8), totalBntBalance)
        })
    })
    describe('Affiliate Fee', async () => {
        it("should throw when passing affiliate account and inappropriate fee", async () => {
            await expectError(
                convertEOS('1.0000', false, undefined, 'BNT', user1, user1, user2, 0),
                "inappropriate affiliate fee"
            )
            await expectError(
               convertEOS('1.0000', false, undefined, 'BNT', user1, user1, user2, -10),
               "inappropriate affiliate fee"
            )
            await expectError(
                convertEOS('1.0000', false, undefined, 'BNT', user1, user1, user2, 9000000),
                "inappropriate affiliate fee"
            )
        })
        it("should throw when passing affiliate account that is not actually an account", async () => {
            await expectError(
                convertEOS('1.0000', false, undefined, 'BNT', user1, user1, "notauser", 3000),
                "affiliate is not an account"
            )
        })
        it("should throw when affiliate fee would bring conversion below min return", async () => {
            await expectError(
                convertEOS('1.0000', false, undefined, 'BNT', user1, user1, user2, 29000, 0.99),
                "below min return"
            )
        })
        it("1 Hop with Affiliate Fee, SingleConverter", async () => {
            var result = await getBalance(user1, bntToken, 'BNT')
            const user1beforeBNT = result.rows[0].balance.split(' ')[0]

            result = await getBalance(user2, bntToken, 'BNT')
            const user2beforeBNT = result.rows[0].balance.split(' ')[0]

            const expectedReturnBeforeFee = 1.00019894
            const expectedReturnAfterFee = 0.97119318

            const feeAmountPaid = expectedReturnBeforeFee - expectedReturnAfterFee

            const res = await expectNoError(
                convertEOS('1.0000', false, undefined, 'BNT', user1, user1, user2, 29000, 0.9),
            )
            const events = res.processed.action_traces[0].inline_traces[2].inline_traces[2].inline_traces[1].console.split("\n")

            result = await getBalance(user1, bntToken, 'BNT')
            const user1afterBNT = result.rows[0].balance.split(' ')[0]

            result = await getBalance(user2, bntToken, 'BNT')
            const user2afterBNT = result.rows[0].balance.split(' ')[0]

            const deltaUser1BNT = parseFloat(user1afterBNT) - parseFloat(user1beforeBNT)
            const deltaUser2BNT = parseFloat(user2afterBNT) - parseFloat(user2beforeBNT)

            assert.equal(deltaUser1BNT.toFixed(8), expectedReturnAfterFee.toFixed(8), 'unexpected return on conversion')
            assert.equal(deltaUser2BNT.toFixed(8), feeAmountPaid.toFixed(8), 'unexpected affiliate fee paid')
            
            const returnEvent = JSON.parse(events[0]).return.split(' ')[0]
            const feeAmountEvent = JSON.parse(events[0]).affiliate_fee.split(' ')[0]
            
            assert.equal(returnEvent, expectedReturnBeforeFee.toFixed(8), 'unexpected return event')
            assert.equal(feeAmountEvent, feeAmountPaid.toFixed(8), 'unexpected affiliate fee event')
        })
        it("Selling BNT is a false positive, should not trigger affiliate fee", async function() {
            var result = await getBalance(user1, 'eosio.token', 'EOS')
            const user1beforeEOS = result.rows[0].balance.split(' ')[0]

            result = await getBalance(user2, bntToken, 'BNT')
            const user2beforeBNT = result.rows[ 0].balance.split(' ')[0]

            const res = await expectNoError(
                convertBNT('1.00000000', 'EOS', undefined, user1, user1, user2, 29000),
            )
            const events = res.processed.action_traces[0].inline_traces[2].inline_traces[1].console.split("\n");
            const expectedReturn = parseFloat(JSON.parse(events[0]).return);

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
    describe('Permissions', async () => {
        it("should throw error when trying to call 'setMultitoken' without proper permissions", async () => {
            await expectError(
                setMultitoken(user1),
                ERRORS.PERMISSIONS
            )
        })
        it("shouldn't throw error when trying to call change settings with proper permissions", async () => {
            await expectNoError(
                setMaxfee(multiConverter, 20000)
            )
            result = await getSettings(multiConverter)
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].max_fee, 20000, "max fee not set correctly - multiconverter")
        })
        it("should throw error when trying to call 'setreserve' without proper permissions", async () => {
            const actor = user2
            await expectError(
                setreserve(true, bntToken, 'BNT', multiConverter, 'TKNA', actor, 200000),
                ERRORS.PERMISSIONS
            )
        })
        it("should throw error when changing converter owner without permission", async () => {
            const actor = user2
            await expectError(
                updateOwner(actor, 'TKNA', user2),
                ERRORS.PERMISSIONS
            )
        })
        it("trying to delete BNT reserve without permission - should throw", async () => { 
            await expectError(
                delreserve('BNT', user2, multiConverter, 'TKNA'), 
                "missing authority of bnttestuser1"
            )
        })
        it("trying to delete BNT reserve when it's not empty - should throw", async () => { 
            await expectError(
                delreserve('BNT', user1, multiConverter, 'TKNA'), 
                "may delete only empty reserves"
            )
        })
        it("trying to delete TKNA converter when reserves not empty - should throw", async () => { 
            await expectError(
                delConverter('TKNA', user1), 
                "delete reserves first"
            )
        })
        it("trying to delete TKNA converter without permission - should throw", async () => { 
            await expectError(
                delConverter('TKNA', user2), 
                "missing authority of bnttestuser1"
            )
        })
        it("shouldn't throw error when changing converter owner with permission", async () => {
            const actor = user2
            await expectNoError(
                updateOwner(actor, 'TKNB', user1)
            )
            result = await getConverter('TKNB')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].owner, user1, "converter owner not set correctly - TKNA")
        })
        it("shouldn't throw error when trying to call 'setreserve' with proper permissions", async () => {
            const actor = user1
            await expectNoError(
                setreserve(true, bntToken, 'BNT', multiConverter, 'TKNA', actor, 200000)
            )
            result = await getReserve('BNT', multiConverter, 'TKNA')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].ratio, 200000, "reserve ratio not set correctly - BNT<->TKNA")
        })
    })
    describe('Formula', async () => {
        it('ensures a proper return amount calculation (no fee) [RESERVE --> SMART]', async () => {
            const inputAmount = '2.20130604'
            
            const initialBalance = Number((await getBalance(user1, multiToken, 'TKNA')).rows[0].balance.split(' ')[0])
            const tokenStats = (await get(multiToken, 'TKNA')).rows[0]
            const reserveData = (await getReserve('BNT', multiConverter, 'TKNA')).rows[0]

            const supply = tokenStats.supply.split(' ')[0]
            const reserveBalance = Number(reserveData.balance.split(' ')[0])
            const { ratio } = reserveData

            const events = await extractEvents(
                convertBNT(inputAmount, 'TKNA', `${multiConverter}:TKNA`)
            )

            const finalBalance = Number((await getBalance(user1, multiToken, 'TKNA')).rows[0].balance.split(' ')[0])
 
            const expectedReturn = Number(calculatePurchaseReturn(supply, reserveBalance, ratio, inputAmount).toFixed(4))
            const actualReturn = Number(new Decimal(finalBalance).sub(initialBalance).toFixed())
            const eventReturn = Number(Number(events.conversion[0].return).toFixed(4))

            assert(parseFloat(Math.abs(actualReturn - expectedReturn).toFixed(4)) <= 0.0001)
            assert(parseFloat(Math.abs(actualReturn - eventReturn).toFixed(4)) <= 0.0001)
        })

        it('ensures a proper return amount calculation (no fee) [RESERVE --> RESERVE]', async () => {
            const inputAmount = randomAmount({min: 0, max: 10, decimals: 8 })
            
            const initialBalance = Number((await getBalance(user1, 'eosio.token', 'EOS')).rows[0].balance.split(' ')[0])
            
            const fromTokenReserveData = (await getReserve('BNT', multiConverter, 'RELAY')).rows[0]
            const toTokenReserveData = (await getReserve('EOS', multiConverter, 'RELAY')).rows[0]
            const fromTokenReserveBalance = Number(fromTokenReserveData.balance.split(' ')[0])
            const toTokenReserveBalance = Number(toTokenReserveData.balance.split(' ')[0])
            
            const events = await extractEvents(
                convertBNT(inputAmount, 'EOS', `${multiConverter}:RELAY`)
            )

            const finalBalance = Number((await getBalance(user1, 'eosio.token', 'EOS')).rows[0].balance.split(' ')[0])
                
            const expectedReturn = Number(calculateQuickConvertReturn(fromTokenReserveBalance, inputAmount, toTokenReserveBalance).toFixed(4))
            const actualReturn = Number(new Decimal(finalBalance).sub(initialBalance).toFixed())
            const eventReturn = Number(Number(events.conversion[0].return).toFixed(4))

            assert(parseFloat(Math.abs(actualReturn - expectedReturn).toFixed(4)) <= 0.0001)
            assert(parseFloat(Math.abs(actualReturn - eventReturn).toFixed(4)) <= 0.0001)
        })
        it('ensures a proper return amount calculation (with fee) [RESERVE --> RESERVE]', async () => {
            const fee = 100;
            await expectNoError(
                updateFee(user1, 'RELAY', fee)
            )
            const inputAmount = randomAmount({min: 0, max: 10, decimals: 8 })
            
            const initialBalance = Number((await getBalance(user1, 'eosio.token', 'EOS')).rows[0].balance.split(' ')[0])
            
            const fromTokenReserveData = (await getReserve('BNT', multiConverter, 'RELAY')).rows[0]
            const toTokenReserveData = (await getReserve('EOS', multiConverter, 'RELAY')).rows[0]
            const fromTokenReserveBalance = Number(fromTokenReserveData.balance.split(' ')[0])
            const toTokenReserveBalance = Number(toTokenReserveData.balance.split(' ')[0])
            
            const events = await extractEvents(
                convertBNT(inputAmount, 'EOS', `${multiConverter}:RELAY`)
            )

            const finalBalance = Number((await getBalance(user1, 'eosio.token', 'EOS')).rows[0].balance.split(' ')[0])
                
            const expectedReturn = Number(calculateQuickConvertReturn(fromTokenReserveBalance, inputAmount, toTokenReserveBalance, fee).toFixed(4))
            const actualReturn = Number(new Decimal(finalBalance).sub(initialBalance).toFixed())
            const eventReturn = Number(Number(events.conversion[0].return).toFixed(4))

            assert(parseFloat(Math.abs(actualReturn - expectedReturn).toFixed(4)) <= 0.0001)
            assert(parseFloat(Math.abs(actualReturn - eventReturn).toFixed(4)) <= 0.0001)

            // Revert to no fee
            await expectNoError(
                updateFee(user1, 'RELAY', 0)
            )
        })
        it('ensures a proper return amount calculation (no fee) [SMART --> RESERVE]', async () => {
            const inputAmount = '4.2213'
            
            const initialBalance = Number((await getBalance(user1, bntToken, 'BNT')).rows[0].balance.split(' ')[0])
            const tokenStats = (await get(multiToken, 'TKNA')).rows[0]
            const reserveData = (await getReserve('BNT', multiConverter, 'TKNA')).rows[0]
            const supply = tokenStats.supply.split(' ')[0]
            const reserveBalance = Number(reserveData.balance.split(' ')[0])
            const { ratio } = reserveData

            const events = await extractEvents(
                convertMulti(inputAmount, 'TKNA', 'BNT')
            )

            const finalBalance = Number((await getBalance(user1, bntToken, 'BNT')).rows[0].balance.split(' ')[0])
 
            const expectedReturn = Number(calculateSaleReturn(supply, reserveBalance, ratio, inputAmount).toFixed(8))
            const actualReturn = Number(new Decimal(finalBalance).sub(initialBalance).toFixed())
            const eventReturn = Number(Number(events.conversion[0].return).toFixed(8))
            assert(parseFloat(Math.abs(actualReturn - expectedReturn).toFixed(8)) <= 0.00000001)
            assert((Math.abs(actualReturn - eventReturn).toFixed(8)) <= 0.00000001)
        })
        it('ensures a proper return amount calculation (with fee) [RESERVE --> SMART]', async () => {
            const inputAmount = '2.20130604'

            const fee = Math.round(Math.random() * 300);
            await expectNoError(
                updateFee(user1, 'TKNB', fee)
            )
            
            const initialBalance = Number((await getBalance(user1, multiToken, 'TKNB')).rows[0].balance.split(' ')[0])
            const tokenStats = (await get(multiToken, 'TKNB')).rows[0]
            const reserveData = (await getReserve('BNT', multiConverter, 'TKNB')).rows[0]

            const supply = tokenStats.supply.split(' ')[0]
            const reserveBalance = Number(reserveData.balance.split(' ')[0])
            const { ratio } = reserveData

            const events = await extractEvents(
                convertBNT(inputAmount, 'TKNB', `${multiConverter}:TKNB`, user1, user1)
            )

            const finalBalance = Number((await getBalance(user1, multiToken, 'TKNB')).rows[0].balance.split(' ')[0])
 
            const expectedReturn = Number(calculatePurchaseReturn(supply, reserveBalance, ratio, inputAmount, fee).toFixed(4))
            const actualReturn = Number(new Decimal(finalBalance).sub(initialBalance).toFixed())
            const eventReturn = Number(Number(events.conversion[0].return).toFixed(4))

            assert(parseFloat(Math.abs(actualReturn - expectedReturn).toFixed(4)) <= 0.0001)
            assert(parseFloat(Math.abs(actualReturn - eventReturn).toFixed(4)) <= 0.0001)
        })
        it('ensures a proper return amount calculation (with fee) [SMART --> RESERVE]', async () => {
            const inputAmount = '4.2213'

            const fee = Math.round(Math.random() * 300);
            await expectNoError(
                updateFee(user1, 'TKNB', fee)
            )
            
            const initialBalance = Number((await getBalance(user1, bntToken, 'BNT')).rows[0].balance.split(' ')[0])
            const tokenStats = (await get(multiToken, 'TKNB')).rows[0]
            const reserveData = (await getReserve('BNT', multiConverter, 'TKNB')).rows[0]
            const supply = tokenStats.supply.split(' ')[0]
            const reserveBalance = Number(reserveData.balance.split(' ')[0])
            const { ratio } = reserveData

            const events = await extractEvents(
                convertMulti(inputAmount, 'TKNB', 'BNT')
            )
            const finalBalance = Number((await getBalance(user1, bntToken, 'BNT')).rows[0].balance.split(' ')[0])
 
            const expectedReturn = Number(calculateSaleReturn(supply, reserveBalance, ratio, inputAmount, fee).toFixed(8))
            const actualReturn = Number(new Decimal(finalBalance).sub(initialBalance).toFixed())
            const eventReturn = Number(Number(events.conversion[0].return).toFixed(8))
            
            assert(parseFloat(Math.abs(actualReturn - expectedReturn).toFixed(8)) <= 0.00000001)
            assert(parseFloat(Math.abs(actualReturn - eventReturn).toFixed(8)) <= 0.00000001)
        })
        it('ensures a proper return amount calculation (with fee) [RESERVE --> RESERVE --> SMART]', async () => {
            const fee = Math.round(Math.random() * 300);

            await expectNoError(
                updateFee(user1, 'RELAY', fee)
            )
            const inputAmount = randomAmount({min: 0, max: 10, decimals: 4 })
            
            const initialBalance = Number((await getBalance(user1, multiToken, 'TKNA')).rows[0].balance.split(' ')[0])
            
            const fromTokenReserveData = (await getReserve('EOS', multiConverter, 'RELAY')).rows[0]
            const toTokenReserveData = (await getReserve('BNT', multiConverter, 'RELAY')).rows[0]
            const fromTokenReserveBalance = Number(fromTokenReserveData.balance.split(' ')[0])
            const toTokenReserveBalance = Number(toTokenReserveData.balance.split(' ')[0])
            
            const tokenStats = (await get(multiToken, 'TKNA')).rows[0]
            const reserveData = (await getReserve('BNT', multiConverter, 'TKNA')).rows[0]

            const secondHopSupply = tokenStats.supply.split(' ')[0]
            const secondHopReserveBalance = Number(reserveData.balance.split(' ')[0])
            const secondHopRatio = reserveData.ratio


            const events = await extractEvents(
                convertTwice(inputAmount, 'eosio.token', 'EOS', 'TKNA', `${multiConverter}:RELAY`, `${multiConverter}:TKNA`)
            )

            const finalBalance = Number((await getBalance(user1, multiToken, 'TKNA')).rows[0].balance.split(' ')[0])
            
            // 1st hop
            const intermediateReturn = Number(calculateQuickConvertReturn(fromTokenReserveBalance, inputAmount, toTokenReserveBalance, fee).toFixed(8))
            
            // 2nd hop
            const expectedReturn = Number(calculatePurchaseReturn(secondHopSupply, secondHopReserveBalance, secondHopRatio, intermediateReturn).toFixed(4))
            
            const actualReturn = Number(new Decimal(finalBalance).sub(initialBalance).toFixed())
            const eventReturn = Number(Number(events.conversion[1].return).toFixed(4))

            assert(parseFloat(Math.abs(actualReturn - expectedReturn).toFixed(4)) <= 0.0001)
            assert(parseFloat(Math.abs(actualReturn - eventReturn).toFixed(4)) <= 0.0001)

            // Revert to no fee
            await expectNoError(
                updateFee(user1, 'RELAY', 0)
            )
        })

        it('ensures a proper return amount calculation (with fee) [SMART --> RESERVE --> RESERVE]', async () => {
            const fee = Math.round(Math.random() * 300);

            await expectNoError(
                updateFee(user1, 'RELAY', fee)
            )
            const inputAmount = randomAmount({min: 0, max: 10, decimals: 4 })
            
            const initialBalance = Number((await getBalance(user1, 'eosio.token', 'EOS')).rows[0].balance.split(' ')[0])
            
            
            const tokenStats = (await get(multiToken, 'TKNA')).rows[0]
            const reserveData = (await getReserve('BNT', multiConverter, 'TKNA')).rows[0]

            const firstHopSupply = tokenStats.supply.split(' ')[0]
            const firstHopReserveBalance = Number(reserveData.balance.split(' ')[0])
            const firstHopRatio = reserveData.ratio

            const secondHopFromTokenReserveData = (await getReserve('BNT', multiConverter, 'RELAY')).rows[0]
            const secondHopToTokenReserveData = (await getReserve('EOS', multiConverter, 'RELAY')).rows[0]
            const secondHopFromTokenReserveBalance = Number(secondHopFromTokenReserveData.balance.split(' ')[0])
            const secondHopToTokenReserveBalance = Number(secondHopToTokenReserveData.balance.split(' ')[0])

            const events = await extractEvents(
                convertTwice(inputAmount, multiToken, 'TKNA', 'EOS', `${multiConverter}:TKNA`, `${multiConverter}:RELAY`)
            )

            const finalBalance = Number((await getBalance(user1, 'eosio.token', 'EOS')).rows[0].balance.split(' ')[0])
            
            // 1st hop
            const intermediateReturn = Number(calculateSaleReturn(firstHopSupply, firstHopReserveBalance, firstHopRatio, inputAmount).toFixed(8))
            // 2nd hop
            const expectedReturn = Number(calculateQuickConvertReturn(secondHopFromTokenReserveBalance, intermediateReturn, secondHopToTokenReserveBalance, fee).toFixed(4))
            
            const actualReturn = Number(new Decimal(finalBalance).sub(initialBalance).toFixed())
            const eventReturn = Number(Number(events.conversion[1].return).toFixed(4))

            assert(parseFloat(Math.abs(actualReturn - expectedReturn).toFixed(4)) <= 0.0001)
            assert(parseFloat(Math.abs(actualReturn - eventReturn).toFixed(4)) <= 0.0001)

            // Revert to no fee
            await expectNoError(
                updateFee(user1, 'RELAY', 0)
            )
        })

        it('ensures a proper return amount calculation (with fee) [RESERVE --> RESERVE --> RESERVE]', async () => {
            const converterAFee = Math.round(Math.random() * 300);
            const converterBFee = Math.round(Math.random() * 300);

            await expectNoError(
                updateFee(user1, 'RELAY', converterAFee)
            )
            await expectNoError(
                updateFee(user1, 'RELAYB', converterBFee)
            )

            const inputAmount = randomAmount({min: 0, max: 10, decimals: 4 })
            
            const initialBalance = Number((await getBalance(user1, 'eosio.token', 'EOS')).rows[0].balance.split(' ')[0])
            
            const firstHopFromTokenReserveData = (await getReserve('EOS', multiConverter, 'RELAY')).rows[0]
            const firstHopToTokenReserveData = (await getReserve('BNT', multiConverter, 'RELAY')).rows[0]
            const firstHopFromTokenReserveBalance = Number(firstHopFromTokenReserveData.balance.split(' ')[0])
            const firstHopToTokenReserveBalance = Number(firstHopToTokenReserveData.balance.split(' ')[0])
            
            const secondHopFromTokenReserveData = (await getReserve('BNT', multiConverter, 'RELAYB')).rows[0]
            const secondHopToTokenReserveData = (await getReserve('EOS', multiConverter, 'RELAYB')).rows[0]
            const secondHopFromTokenReserveBalance = Number(secondHopFromTokenReserveData.balance.split(' ')[0])
            const secondHopToTokenReserveBalance = Number(secondHopToTokenReserveData.balance.split(' ')[0])

            const events = await extractEvents(
                convertTwice(inputAmount, 'eosio.token', 'EOS', 'EOS', `${multiConverter}:RELAY`, `${multiConverter}:RELAYB`)
            )

            const finalBalance = Number((await getBalance(user1, 'eosio.token', 'EOS')).rows[0].balance.split(' ')[0])
            
            // 1st hop
            const intermediateReturn = Number(calculateQuickConvertReturn(firstHopFromTokenReserveBalance, inputAmount, firstHopToTokenReserveBalance, converterAFee).toFixed(8))
            
            // 2nd hop
            const expectedReturn = Number(calculateQuickConvertReturn(secondHopFromTokenReserveBalance, intermediateReturn, secondHopToTokenReserveBalance, converterBFee).toFixed(4))
            
            const actualReturn = Number(new Decimal(finalBalance).sub(initialBalance).add(inputAmount).toFixed())
            const eventReturn = Number(Number(events.conversion[1].return).toFixed(4))

            assert(parseFloat(Math.abs(actualReturn - expectedReturn).toFixed(4)) <= 0.0001)
            assert(parseFloat(Math.abs(actualReturn - eventReturn).toFixed(4)) <= 0.0001)

            // Revert to no fee
            await expectNoError(
                updateFee(user1, 'RELAY', 0)
            )
            
            // Revert to no fee
            await expectNoError(
                updateFee(user1, 'RELAYB', 0)
            )
        })

        it("funding: accept more deposits", async() => {
            // Reserves are as expected
            result = await getReserve('BNT', multiConverter, 'BNTEOS')
            var balance = result.rows[0].balance
            assert.equal(balance, '990.00000001 BNT', 'BNT reserve balance not as expected')
            result = await getReserve('EOS', multiConverter, 'BNTEOS')
            balance = result.rows[0].balance
            assert.equal(balance, '990.0001 EOS', "EOS reserve balance not as expected")

            await expectNoError( 
                transfer(bntToken, '10.00000000 BNT', multiConverter, user1, 'fund;BNTEOS') 
            )
            await expectNoError( 
                transfer('eosio.token', '10.0000 EOS', multiConverter, user1, 'fund;BNTEOS') 
            )

            result = await getAccount(user1, 'BNTEOS', 'EOS')
            var balance = result.rows[0].quantity.split(' ')[0]
            assert.equal(balance, '10.0000', 'EOS account balance invalid')

            result = await getAccount(user1, 'BNTEOS', 'BNT')
            var balance = result.rows[0].quantity.split(' ')[0]
            assert.equal(balance, '10.00000000', 'BNT account balance invalid')
        })

        it("funding: charges as expected", async() => {


            const [userEosBefore, userBntBefore] = await Promise.all([getAccount(user1, 'BNTEOS', 'EOS'), getAccount(user1, 'BNTEOS', 'BNT')])

            await expectNoError(
                fund(user1, '148.5000 BNTEOS')
            )

            result = await getAccount(user1, 'BNTEOS', 'EOS')
            var balance = result.rows[0].quantity.split(' ')[0]
            assert.equal(balance, Number(userEosBefore.rows[0].quantity.split(' ')[0]) - 1.4851, 'EOS balance invalid')
            
            result = await getAccount(user1, 'BNTEOS', 'BNT')
            var balance = result.rows[0].quantity.split(' ')[0]
            assert.equal(balance, Number(userBntBefore.rows[0].quantity.split(' ')[0]) - 1.48500001, 'BNT balance invalid')


            const [userEosBefore2, userBntBefore2] = await Promise.all([getAccount(user1, 'BNTEOS', 'EOS'), getAccount(user1, 'BNTEOS', 'BNT')])

            await expectNoError(
                fund(user1, '657.9450 BNTEOS')
            )

            result = await getAccount(user1, 'BNTEOS', 'EOS')
            var balance = result.rows[0].quantity.split(' ')[0]
            assert.equal(balance, new Decimal(Number(userEosBefore2.rows[0].quantity.split(' ')[0])).minus(6.5795).toString(), 'EOS balance invalid')
            
            result = await getAccount(user1, 'BNTEOS', 'BNT')
            var balance = result.rows[0].quantity.split(' ')[0]
            assert.equal(balance, new Decimal(Number(userBntBefore2.rows[0].quantity.split(' ')[0])).minus(6.57945001).toString(), 'BNT balance invalid')

        })

    })

    describe('Input validations', async () => {
        it('[fund] ensures assets with invalid precision are rejected', async () => {
            await expectError( // last fund cleared the accounts row
                fund(user1, '10000.0 BNTEOS'),
                "symbol mismatch"
            )
            await expectError( // last fund cleared the accounts row
                fund(user1, '10000.00000001 TKNA'),
                "symbol mismatch"
            )
        });
        it('[liquidate] ensures assets with invalid precision are rejected', async () => {
            await expectError( 
                transfer('bnt2eosrelay',  '1.00000000 BNTEOS', multiConverter ,user1, 'liquidate'),
                'bad origin for this transfer'
            )
        });
    });
})
