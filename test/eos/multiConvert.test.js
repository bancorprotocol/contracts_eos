
const Decimal = require('decimal.js')
const chai = require('chai')
const assert = chai.assert

const {
    expectError,
    expectNoError,
    randomAmount,
    extractEvents,
    calculatePurchaseReturn,
    calculateSaleReturn
} = require('./common/utils')

const {
    get,
    transfer,
    getBalance,
    convertBNT,
    convertUGT
} = require('./common/token')

const {
    createConverter,
    setMultitoken,
    enableConvert,
    getConverter,
    setreserve,
    getReserve,
    getSettings,
    setEnabled,
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
            await expectNoError(
                setEnabled(multiConverter)
            )
            result = await getSettings(multiConverter)
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].enabled, true, "multiconverter not enabled")
            await expectNoError(
                setStaking(multiConverter)
            )
            await expectNoError(
                setMaxfee(multiConverter, 30000)
            )
        })
        it('setup converters', async function() {
            let AinitSupply = '1000.0000 UGTTKNA'
            let AmaxSupply = '100000030.0096 UGTTKNA'
            
            let BinitSupply = '1000.0000 UGTTKNB'
            let BmaxSupply = '10002012.1090 UGTTKNB'

            let CinitSupply = '99000.00000000 BNTEOS'
            let CmaxSupply = '100000000.00000000 BNTEOS'

            await expectNoError(
                createConverter(user1, AinitSupply, AmaxSupply) 
            )
            result = await getConverter('UGTTKNA')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].fee, 0, "converter fee not set correctly - UGTTKNA")

            await expectNoError(
                createConverter(user2, BinitSupply, BmaxSupply) 
            )
            await expectNoError(
                createConverter(user2, CinitSupply, CmaxSupply) 
            )
        })
        it('setup reserves', async function() {
            let Aratio = 100000
            let Bratio = 300000
            let Cratio = 500000
            await expectNoError(
                setreserve(true, bntToken, 'BNT', multiConverter, 'UGTTKNA', user1, Aratio) 
            )
            await expectNoError(
                setreserve(true, bntToken, 'BNT', multiConverter, 'UGTTKNB', user2, Bratio) 
            )
            await expectNoError(
                setreserve(true, bntToken, 'BNT', multiConverter, 'BNTEOS', user2, Cratio) 
            )
            await expectNoError(
                setreserve(false, 'eosio.token', 'EOS', multiConverter, 'BNTEOS', user2, Cratio) 
            )
        })
        it('fund reserves', async function () {        
            //just for opening accounts
            await expectNoError(
                transfer(multiToken, '0.0001 UGTTKNA', user2, user1, "")
            )
            await expectNoError(
                transfer(multiToken, '0.0001 UGTTKNB', user1, user2, "")
            )

            await expectNoError( 
                transfer(bntToken, '1000.00000000 BNT', multiConverter, user1, 'setup;UGTTKNA') 
            )
            await expectNoError( 
                transfer(bntToken, '1000.00000000 BNT', multiConverter, user2, 'setup;UGTTKNB') 
            )
            await expectNoError( 
                transfer(bntToken, '999.00000000 BNT', multiConverter, user1, 'setup;BNTEOS') 
            )
            await expectNoError( 
                transfer('eosio.token', '990.0000 EOS', multiConverter, user1, 'setup;BNTEOS') 
            )
            await expectNoError(
                withdraw(user1, '9.00000000 BNT', 'BNTEOS')
            )
            result = await getReserve('BNT', multiConverter, 'BNTEOS')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].balance, '990.00000000 BNT', "withdrawal invalid - BNTEOS")

            result = await getReserve('EOS', multiConverter, 'BNTEOS')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].balance, '990.0000 EOS', "funded amount invalid - BNTEOS")
        })
        it("should throw error when trying to convert while disabled", async () => {
            await expectError(
                transfer(bntToken, `${randomAmount({min: 8, max: 12, decimals: 8 })} BNT`, 'thisisbancor', user1, 
                                 `1,${multiConverter}:UGTTKNA UGTTKNA,0.0001,${user1}`
                        ), 
                ERRORS.CONVERSIONS_DISABLED
            )
            await expectError(
                transfer(bntToken, `${randomAmount({min: 8, max: 12, decimals: 8 })} BNT`, 'thisisbancor', user1, 
                                 `1,${multiConverter}:UGTTKNB UGTTKNB,0.0001,${user1}`
                        ),
                ERRORS.CONVERSIONS_DISABLED
            )
            let Bfee = 10000
            await expectNoError(
                updateFee(user2, 'UGTTKNB', Bfee)
            )
            result = await getConverter('UGTTKNB')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].fee, Bfee, "fee not properly set - UGTTKNA")
        })
        it("enable converters and staking", async () => { 
            await expectNoError(
               enableConvert(user1, 'UGTTKNA')
            )
            result = await getConverter('UGTTKNA')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].enabled, true, "converter not enabled - UGTTKNA")
            
            await expectNoError(
                enableStake(user2, 'BNTEOS')
            )
            result = await getConverter('BNTEOS')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].stake_enabled, true, "converter not stake_enabled - UGTTKNA")
            await expectError(
                updateFee(user1, 'BNTEOS', 10),
                ERRORS.PERMISSIONS
            )
            await expectNoError(
                enableConvert(user2, 'UGTTKNB')
            )
            await expectNoError(
                enableConvert(user2, 'BNTEOS')
            )
            await expectError(
                transfer(bntToken, '1000.00000000 BNT', multiConverter, user1, 'setup;BNTEOS'),
                ERRORS.SETUP 
            )
            await expectError(
                withdraw(user1, '9.00000000 BNT', 'BNTEOS'),
                ERRORS.SETUP 
            )    
        })
        it('ensures that fund and liquidate are working properly', async () => {
            bntReserveBefore = '990.00000000 BNT'
            var result = await await getBalance(user1, bntToken, 'BNT')
            var bntUserBefore = result.rows[0].balance.split(' ')[0]
            await expectNoError(
                transfer(bntToken, '10.00000000 BNT', multiConverter, user1, 'fund;BNTEOS'),
            )
            result = await getReserve('BNT', multiConverter, 'BNTEOS')
            var bntReserveAfter = result.rows[0].balance
            assert.equal(bntReserveBefore, bntReserveAfter, 'BNT wasnt supposed to change yet - BNTEOS')
            
            result = await await getBalance(user1, bntToken, 'BNT')
            var bntUserAfter = result.rows[0].balance.split(' ')[0]
            assert.equal(parseFloat(bntUserBefore) - parseFloat(bntUserAfter), 10.0000, 'BNT balance didnt go down - BNTEOS')
           
            eosReserveBefore = '990.0000 EOS'
            await expectNoError( 
                transfer('eosio.token', '10.0000 EOS', multiConverter, user1, 'fund;BNTEOS') 
            )
            result = await getReserve('EOS', multiConverter, 'BNTEOS')
            var eosReserveAfter = result.rows[0].balance
            assert.equal(eosReserveBefore, eosReserveAfter, 'EOS wasnt supposed to change yet - BNTEOS')
                
            await expectNoError(
                fund(user1, '1000.00000000 BNTEOS')
            )
            
            result = await getBalance(user1, multiToken, 'BNTEOS')
            assert.equal(result.rows.length, 1)
            let relayBalance = result.rows[0].balance
            assert.equal(relayBalance, '1000.00000000 BNTEOS', 'incorrect BNTEOS issued')

            result = await getReserve('BNT', multiConverter, 'BNTEOS')
            bntReserveAfter = result.rows[0].balance
            assert.equal(bntReserveAfter, '1000.00000000 BNT', 'BNT was supposed to change - BNTEOS')

            result = await getReserve('EOS', multiConverter, 'BNTEOS')
            eosReserveAfter = result.rows[0].balance
            assert.equal(eosReserveAfter, '1000.0000 EOS', 'EOS was supposed to change - BNTEOS')
            
            result = await getBalance(user1, bntToken, 'BNT')
            bntUserBefore = result.rows[0].balance.split(' ')[0]
            result = await getBalance(user1, 'eosio.token', 'EOS')
            let eosUserBefore = result.rows[0].balance.split(' ')[0]

            await expectNoError( 
                transfer(multiToken, relayBalance, multiConverter, user1, 'liquidate')
            )
        
            result = await getBalance(user1, bntToken, 'BNT')
            bntUserAfter = result.rows[0].balance.split(' ')[0]
            assert.equal(10, bntUserAfter - bntUserBefore, 'user bnt balance after is incorrect - BNTEOS')
            
            result = await getBalance(user1, 'eosio.token', 'EOS')
            let eosUserAfter = result.rows[0].balance.split(' ')[0]
            assert.equal(10, eosUserAfter - eosUserBefore, result.rows[0].balance, 0, 'user eos balance after is incorrect - BNTEOS')

            result = await getReserve('BNT', multiConverter, 'BNTEOS')
            bntReserveAfter = result.rows[0].balance.split(' ')[0]
            assert.equal(bntReserveAfter, '990.00000000', 'BNT reserve after is incorrect - BNTEOS')

            result = await getReserve('EOS', multiConverter, 'BNTEOS')
            eosReserveAfter = result.rows[0].balance.split(' ')[0]
            assert.equal(eosReserveAfter, '990.0000', 'EOS reserve after is incorrect - BNTEOS')

            result = await getBalance(user1, multiToken, 'BNTEOS')
            assert.equal(result.rows[0].balance, '0.00000000 BNTEOS', 'smart token balance after is incorrect - BNTEOS')
        })
        it('ensures that converters balances sum is equal to the multiConverter\'s total BNT balance [Load Test]', async function () {
            this.timeout(10000)
            const bntTxs = []
            const ugtTxs = []
            for (let i = 0; i < 5; i++) {
                bntTxs.push(
                    transfer(bntToken, `${randomAmount({min: 8, max: 12, decimals: 8 })} BNT`, 'thisisbancor', user1, 
                                     `1,${multiConverter}:UGTTKNA UGTTKNA,0.0001,${user1}`
                            ),
                    transfer(bntToken, `${randomAmount({min: 8, max: 12, decimals: 8 })} BNT`, 'thisisbancor', user2, 
                                     `1,${multiConverter}:UGTTKNB UGTTKNB,0.0001,${user2}`
                            ),
                )
                ugtTxs.push(
                    transfer(multiToken, `${randomAmount({min: 1, max: 8, decimals: 4 })} UGTTKNA`, 'thisisbancor', user1, 
                                       `1,${multiConverter}:UGTTKNA BNT,0.0000000001,${user1}`
                            ),
                    transfer(multiToken, `${randomAmount({min: 1, max: 8, decimals: 4 })} UGTTKNB`, 'thisisbancor', user2, 
                                       `1,${multiConverter}:UGTTKNB BNT,0.0000000001,${user2}`
                            ),
                    transfer(multiToken, `${randomAmount({min: 1, max: 8, decimals: 4 })} UGTTKNA`, 'thisisbancor', user1, 
                                       `1,${multiConverter}:UGTTKNA BNT ${multiConverter}:UGTTKNB UGTTKNB,0.0001,${user1}`
                            ),
                    transfer(multiToken, `${randomAmount({min: 1, max: 8, decimals: 4 })} UGTTKNB`, 'thisisbancor', user2, 
                                       `1,${multiConverter}:UGTTKNB BNT ${multiConverter}:UGTTKNA UGTTKNA,0.0001,${user2}`
                            )
                )
            }
            await Promise.all(bntTxs)
            await Promise.all(ugtTxs)

            const TokenAReserveBalance = Number((await getReserve('BNT', multiConverter, 'UGTTKNA')).rows[0].balance.split(' ')[0])
            const TokenBReserveBalance = Number((await getReserve('BNT', multiConverter, 'UGTTKNB')).rows[0].balance.split(' ')[0])

            const totalBntBalance = (await getBalance(multiConverter, bntToken, 'BNT')).rows[0].balance.split(' ')[0]
            assert.equal((TokenAReserveBalance + TokenBReserveBalance + 990).toFixed(8), totalBntBalance)
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
                setreserve(true, bntToken, 'BNT', multiConverter, 'UGTTKNA', actor, 200000),
                ERRORS.PERMISSIONS
            )
        })
        it("should throw error when changing converter owner without permission", async () => {
            const actor = user2
            await expectError(
                updateOwner(actor, 'UGTTKNA', user2),
                ERRORS.PERMISSIONS
            )
        })
        it("shouldn't throw error when changing converter owner with permission", async () => {
            const actor = user2
            await expectNoError(
                updateOwner(actor, 'UGTTKNB', user1)
            )
            result = await getConverter('UGTTKNB')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].owner, user1, "converter owner not set correctly - UGTTKNA")
        })
        it("shouldn't throw error when trying to call 'setreserve' with proper permissions", async () => {
            const actor = user1
            await expectNoError(
                setreserve(true, bntToken, 'BNT', multiConverter, 'UGTTKNA', actor, 200000)
            )
            result = await getReserve('BNT', multiConverter, 'UGTTKNA')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].ratio, 200000, "reserve ratio not set correctly - BNT<->UGTTKNA")
        })
    })
    describe('Formula', async () => {
        it('ensures a proper return amount calculation (no fee) [RESERVE --> SMART]', async () => {
            const inputAmount = '2.20130604'
            
            const initialBalance = Number((await getBalance(user1, multiToken, 'UGTTKNA')).rows[0].balance.split(' ')[0])
            const tokenStats = (await get(multiToken, 'UGTTKNA')).rows[0]
            const reserveData = (await getReserve('BNT', multiConverter, 'UGTTKNA')).rows[0]

            const supply = tokenStats.supply.split(' ')[0]
            const reserveBalance = Number(reserveData.balance.split(' ')[0])
            const { ratio } = reserveData

            const events = await extractEvents(
                convertBNT(inputAmount, 'UGTTKNA', `${multiConverter}:UGTTKNA`)
            )

            const finalBalance = Number((await getBalance(user1, multiToken, 'UGTTKNA')).rows[0].balance.split(' ')[0])
 
            const expectedReturn = Number(calculatePurchaseReturn(supply, reserveBalance, ratio, inputAmount).toFixed(4))
            const actualReturn = Number(new Decimal(finalBalance).sub(initialBalance).toFixed())
            const eventReturn = Number(Number(events.conversion.return).toFixed(4))

            assert(parseFloat(Math.abs(actualReturn - expectedReturn).toFixed(4)) <= 0.0001)
            assert(parseFloat(Math.abs(actualReturn - eventReturn).toFixed(4)) <= 0.0001)
        })
        it('ensures a proper return amount calculation (no fee) [SMART --> RESERVE]', async () => {
            const inputAmount = '4.2213'
            
            const initialBalance = Number((await getBalance(user1, bntToken, 'BNT')).rows[0].balance.split(' ')[0])
            const tokenStats = (await get(multiToken, 'UGTTKNA')).rows[0]
            const reserveData = (await getReserve('BNT', multiConverter, 'UGTTKNA')).rows[0]
            const supply = tokenStats.supply.split(' ')[0]
            const reserveBalance = Number(reserveData.balance.split(' ')[0])
            const { ratio } = reserveData

            const events = await extractEvents(
                convertUGT(inputAmount, 'UGTTKNA', 'BNT')
            )

            const finalBalance = Number((await getBalance(user1, bntToken, 'BNT')).rows[0].balance.split(' ')[0])
 
            const expectedReturn = Number(calculateSaleReturn(supply, reserveBalance, ratio, inputAmount).toFixed(8))
            const actualReturn = Number(new Decimal(finalBalance).sub(initialBalance).toFixed())
            const eventReturn = Number(Number(events.conversion.return).toFixed(8))
            assert(parseFloat(Math.abs(actualReturn - expectedReturn).toFixed(8)) <= 0.00000001)
            assert((Math.abs(actualReturn - eventReturn).toFixed(8)) <= 0.00000001)
        })
        it('ensures a proper return amount calculation (with fee) [RESERVE --> SMART]', async () => {
            const inputAmount = '2.20130604'

            const fee = 10000
            
            const initialBalance = Number((await getBalance(user1, multiToken, 'UGTTKNB')).rows[0].balance.split(' ')[0])
            const tokenStats = (await get(multiToken, 'UGTTKNB')).rows[0]
            const reserveData = (await getReserve('BNT', multiConverter, 'UGTTKNB')).rows[0]

            const supply = tokenStats.supply.split(' ')[0]
            const reserveBalance = Number(reserveData.balance.split(' ')[0])
            const { ratio } = reserveData

            const events = await extractEvents(
                convertBNT(inputAmount, 'UGTTKNB', `${multiConverter}:UGTTKNB`, user1, user1)
            )

            const finalBalance = Number((await getBalance(user1, multiToken, 'UGTTKNB')).rows[0].balance.split(' ')[0])
 
            const expectedReturn = Number(calculatePurchaseReturn(supply, reserveBalance, ratio, inputAmount, fee).toFixed(4))
            const actualReturn = Number(new Decimal(finalBalance).sub(initialBalance).toFixed())
            const eventReturn = Number(Number(events.conversion.return).toFixed(4))

            assert(parseFloat(Math.abs(actualReturn - expectedReturn).toFixed(4)) <= 0.0001)
            assert(parseFloat(Math.abs(actualReturn - eventReturn).toFixed(4)) <= 0.0001)
        })
        it('ensures a proper return amount calculation (with fee) [SMART --> RESERVE]', async () => {
            const inputAmount = '4.2213'

            const fee = 10000
            
            const initialBalance = Number((await getBalance(user1, bntToken, 'BNT')).rows[0].balance.split(' ')[0])
            const tokenStats = (await get(multiToken, 'UGTTKNB')).rows[0]
            const reserveData = (await getReserve('BNT', multiConverter, 'UGTTKNB')).rows[0]
            const supply = tokenStats.supply.split(' ')[0]
            const reserveBalance = Number(reserveData.balance.split(' ')[0])
            const { ratio } = reserveData

            const events = await extractEvents(
                convertUGT(inputAmount, 'UGTTKNB', 'BNT')
            )
            const finalBalance = Number((await getBalance(user1, bntToken, 'BNT')).rows[0].balance.split(' ')[0])
 
            const expectedReturn = Number(calculateSaleReturn(supply, reserveBalance, ratio, inputAmount, fee).toFixed(8))
            const actualReturn = Number(new Decimal(finalBalance).sub(initialBalance).toFixed())
            const eventReturn = Number(Number(events.conversion.return).toFixed(8))
            
            assert(parseFloat(Math.abs(actualReturn - expectedReturn).toFixed(8)) <= 0.00000001)
            assert(parseFloat(Math.abs(actualReturn - eventReturn).toFixed(8)) <= 0.00000001)
        })
    })
})