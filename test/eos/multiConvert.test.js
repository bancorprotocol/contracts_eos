
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
    convertMulti
} = require('./common/token')

const {
    createConverter,
    delConverter,
    setMultitoken,
    enableConvert,
    getConverter,
    getAccount,
    setreserve,
    delreserve,
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
            assert.equal(result.rows[0].active, true, "multiconverter not enabled")
            await expectNoError(
                setStaking(multiConverter)
            )
            await expectNoError(
                setMaxfee(multiConverter, 30000)
            )
        })
        it('setup converters', async function() {
            const AinitSupply = '1000.0000'
            const AmaxSupply = '100000030.0096'
            const Asymbol = 'TKNA'
            
            const BinitSupply = '1000.0000'
            const BmaxSupply = '10002012.1090'
            const Bsymbol = 'TKNB'

            const CinitSupply = '99000.00000000'
            const CmaxSupply = '100000000.00000000'
            const Csymbol = 'BNTEOS'

            await expectNoError(
                createConverter(user1, Asymbol, AinitSupply, AmaxSupply) 
            )
            result = await getConverter(Asymbol)
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].fee, 0, "converter fee not set correctly - TKNA")

            await expectNoError(
                createConverter(user2, Bsymbol, BinitSupply, BmaxSupply) 
            )
            await expectNoError(
                createConverter(user1, Csymbol, CinitSupply, CmaxSupply) 
            )
        })
        it('setup reserves', async function() {
            let Aratio = 100000
            let Bratio = 300000
            let Cratio = 500000
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
                transfer(bntToken, '999.00000000 BNT', multiConverter, user1, 'fund;BNTEOS') 
            )
            await expectNoError( 
                transfer('eosio.token', '990.0000 EOS', multiConverter, user1, 'fund;BNTEOS') 
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
                withdraw(user2, '9.00000000 BNT', 'BNTEOS'),
                "only converter owner may fund/withdraw prior to activation"
            )
            await expectError(
                withdraw(user1, '1000.00000000 BNT', 'BNTEOS'),
                "insufficient amount in reserve"
            )
            await expectError(
                transfer(bntToken, `${randomAmount({min: 8, max: 12, decimals: 8 })} BNT`, 'thisisbancor', user1, 
                                 `1,${multiConverter}:TKNA TKNA,0.0001,${user1}`
                        ), 
                ERRORS.CONVERSIONS_DISABLED
            )
            await expectError(
                transfer(bntToken, `${randomAmount({min: 8, max: 12, decimals: 8 })} BNT`, 'thisisbancor', user1, 
                                 `1,${multiConverter}:TKNB TKNB,0.0001,${user1}`
                        ),
                ERRORS.CONVERSIONS_DISABLED
            )
            let Bfee = 10000
            await expectNoError(
                updateFee(user2, 'TKNB', Bfee)
            )
            result = await getConverter('TKNB')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].fee, Bfee, "fee not properly set - TKNA")
        })
        it("enable converters and staking", async () => { 
            await expectNoError(
               enableConvert(user1, 'TKNA')
            )
            result = await getConverter('TKNA')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].enabled, true, "converter not enabled - TKNA")
            
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
            await expectNoError(
                enableConvert(user2, 'TKNB')
            )
            await expectNoError(
                enableConvert(user1, 'BNTEOS')
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
                transfer(bntToken, '9.00000000 BNT', multiConverter, user1, 'fund;BNTEOS'),
            )
            result = await getAccount(user1, 'BNTEOS', 'BNT')
            balance = result.rows[0].quantity.split(' ')[0]
            assert.equal(balance, '10.00000000', 'BNT wasnt supposed to change yet - BNTEOS')

            result = await getBalance(user1, bntToken, 'BNT')
            var bntUserAfter = result.rows[0].balance.split(' ')[0]
            assert.equal(parseFloat(bntUserBefore) - parseFloat(bntUserAfter), 10.0000, 'BNT balance didnt go down appropriately - BNTEOS')
           
            eosReserveBefore = '990.0000 EOS'
            await expectNoError( 
                transfer('eosio.token', '10.0000 EOS', multiConverter, user1, 'fund;BNTEOS') 
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
                fund(user1, '2000.00000000 BNTEOS'),
                "insufficient balance"
            )
            await expectNoError(
                fund(user1, '1000.00000000 BNTEOS')
            )

            eosTemp = await getAccount(user1, "BNTEOS", "EOS");
            bntTemp = await getAccount(user1, "BNTEOS", "BNT");
            assert.equal(eosTemp.rows.length, 0, "was expecting to have no bank balance")
            assert.equal(bntTemp.rows.length, 0, "was expecting to have no bank balance")

            await expectError( // last fund cleared the accounts row
                fund(user1, '10000.00000000 BNTEOS'),
                "cannot withdraw non-existant deposit"
            )
            
            // Fetch users balance and confirm he was awarded the 10000 BNT he asked for
            result = await getBalance(user1, multiToken, 'BNTEOS')
            assert.equal(result.rows.length, 1)
            let userSmartTokenBalanceAfter = parseFloat(result.rows[0].balance)
            let delta = (userSmartTokenBalanceAfter - userSmartTokenBalanceBefore).toFixed(8)
            assert.equal(delta, '1000.00000000', 'incorrect BNTEOS issued')
            
            // Get relay reserve - BNT
            result = await getReserve('BNT', multiConverter, 'BNTEOS')
            bntReserveAfter = result.rows[0].balance
            assert.equal(bntReserveAfter, '1000.00000000 BNT', 'BNT was supposed to change - BNTEOS')

            // Get Relay reserve - EOS
            result = await getReserve('EOS', multiConverter, 'BNTEOS')
            eosReserveAfter = result.rows[0].balance
            assert.equal(eosReserveAfter, '1000.0000 EOS', 'EOS was supposed to change - BNTEOS')
            
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
                transfer(multiToken, '1000.00000000 BNTEOS', multiConverter, user1, 'liquidate')
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
            let userRelayAfter = result.rows[0].balance.split(' ')[0]
            assert.equal((userRelayBefore - userRelayAfter).toFixed(8), '1000.00000000', 'smart token balance after is incorrect - BNTEOS')
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
            const eventReturn = Number(Number(events.conversion.return).toFixed(4))

            assert(parseFloat(Math.abs(actualReturn - expectedReturn).toFixed(4)) <= 0.0001)
            assert(parseFloat(Math.abs(actualReturn - eventReturn).toFixed(4)) <= 0.0001)
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
            const eventReturn = Number(Number(events.conversion.return).toFixed(8))
            assert(parseFloat(Math.abs(actualReturn - expectedReturn).toFixed(8)) <= 0.00000001)
            assert((Math.abs(actualReturn - eventReturn).toFixed(8)) <= 0.00000001)
        })
        it('ensures a proper return amount calculation (with fee) [RESERVE --> SMART]', async () => {
            const inputAmount = '2.20130604'

            const fee = 10000
            
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
            const eventReturn = Number(Number(events.conversion.return).toFixed(4))

            assert(parseFloat(Math.abs(actualReturn - expectedReturn).toFixed(4)) <= 0.0001)
            assert(parseFloat(Math.abs(actualReturn - eventReturn).toFixed(4)) <= 0.0001)
        })
        it('ensures a proper return amount calculation (with fee) [SMART --> RESERVE]', async () => {
            const inputAmount = '4.2213'

            const fee = 10000
            
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
            const eventReturn = Number(Number(events.conversion.return).toFixed(8))
            
            assert(parseFloat(Math.abs(actualReturn - expectedReturn).toFixed(8)) <= 0.00000001)
            assert(parseFloat(Math.abs(actualReturn - eventReturn).toFixed(8)) <= 0.00000001)
        })


        it("funding: accept more deposits", async() => {
            // Reserves are as expected
            result = await getReserve('BNT', multiConverter, 'BNTEOS')
            var balance = result.rows[0].balance
            assert.equal(balance, '990.00000000 BNT', 'EOS reserve balance not as expected')
            result = await getReserve('EOS', multiConverter, 'BNTEOS')
            balance = result.rows[0].balance
            assert.equal(balance, '990.0000 EOS', "BNT reserve balance not as expected")

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

        it("funding: should reject a fund amount too small to award", async() => {
            await expectError(
                fund(user1, '0.00009000 BNTEOS'),
                "fund amount too small"
            )
        })

        it("funding: charges as expected", async() => {


            let [userEosBefore, userBntBefore] = await Promise.all([getAccount(user1, 'BNTEOS', 'EOS'), getAccount(user1, 'BNTEOS', 'BNT')])

            await expectNoError(
                fund(user1, '148.50000000 BNTEOS')
            )

            result = await getAccount(user1, 'BNTEOS', 'EOS')
            var balance = result.rows[0].quantity.split(' ')[0]
            assert.equal(balance, Number(userEosBefore.rows[0].quantity.split(' ')[0]) - 1.485, 'EOS balance invalid')
            
            result = await getAccount(user1, 'BNTEOS', 'EOS')
            var balance = result.rows[0].quantity.split(' ')[0]
            assert.equal(balance, Number(userBntBefore.rows[0].quantity.split(' ')[0]) - 1.485, 'EOS balance invalid')


            const [userEosBefore2, userBntBefore2] = await Promise.all([getAccount(user1, 'BNTEOS', 'EOS'), getAccount(user1, 'BNTEOS', 'BNT')])

            await expectNoError(
                fund(user1, '657.94500123 BNTEOS')
            )

            result = await getAccount(user1, 'BNTEOS', 'EOS')
            var balance = result.rows[0].quantity.split(' ')[0]
            assert.equal(balance, new Decimal(Number(userEosBefore2.rows[0].quantity.split(' ')[0])).minus(6.5794).toString(), 'EOS balance invalid')
            
            result = await getAccount(user1, 'BNTEOS', 'EOS')
            var balance = result.rows[0].quantity.split(' ')[0]
            assert.equal(balance, new Decimal(Number(userBntBefore2.rows[0].quantity.split(' ')[0])).minus(6.5794), 'EOS balance invalid')

        })

    })
})
