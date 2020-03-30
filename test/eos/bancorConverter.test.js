const Decimal = require('decimal.js')
const chai = require('chai')
const assert = chai.assert

const config = require('../../config/accountNames.json')
const {
    expectError,
    expectNoError,
    randomAmount,
    extractEvents,
    calculatePurchaseReturn,
    calculateSaleReturn,
    calculateQuickConvertReturn,
    calculateFundCost,
    calculateLiquidateReturn,
    toFixedRoundUp,
    toFixedRoundDown
} = require('./common/utils')

const {
    get,
    transfer,
    getBalance,
    convertBNT,
    convert,
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
    enableStake,
    updateOwner,
    updateFee,
    setMaxfee,
    withdraw,
    fund
} = require('./common/converter')

const { ERRORS } = require('./common/errors')

const bancorNetwork = config.BANCOR_NETWORK_ACCOUNT
const bntToken = config.BNT_TOKEN_ACCOUNT
const user1 = config.MASTER_ACCOUNT
const user2 = config.TEST_ACCOUNT
const multiToken = config.MULTI_TOKEN_ACCOUNT
const bancorConverter = config.BANCOR_CONVERTER_ACCOUNT

describe('BancorConverter', () => {
    describe('setup', async () => {
        it('setup converters', async function() {
            const AinitSupply = '1000.0000'
            const Asymbol = 'TKNA'
            
            const BinitSupply = '1000.0000'
            const Bsymbol = 'TKNB'

            const CinitSupply = '99000.00000000'
            const Csymbol = 'BNTSYS'

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
                setreserve(true, bntToken, 'BNT', bancorConverter, 'TKNA', user1, Aratio) 
            )

            await expectNoError(
                setreserve(true, bntToken, 'BNT', bancorConverter, 'TKNB', user2, Bratio) 
            )

            await expectNoError(
                setreserve(true, bntToken, 'BNT', bancorConverter, 'BNTSYS', user1, Cratio) 
            )
            await expectNoError(
                setreserve(false, 'eosio.token', 'SYS', bancorConverter, 'BNTSYS', user1, Cratio) 
            )

            await expectNoError(
                setreserve(true, bntToken, 'BNT', bancorConverter, 'RELAY', user1, Dratio) 
            )
            await expectNoError(
                setreserve(false, 'eosio.token', 'EOS', bancorConverter, 'RELAY', user1, Dratio) 
            )

            await expectNoError(
                setreserve(true, bntToken, 'BNT', bancorConverter, 'RELAYB', user1, Eratio) 
            )
            await expectNoError(
                setreserve(false, 'eosio.token', 'EOS', bancorConverter, 'RELAYB', user1, Eratio) 
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
                transfer(bntToken, '1000.00000000 BNT', bancorConverter, user1, 'fund;TKNA') 
            )
            await expectNoError( 
                transfer(bntToken, '1000.00000000 BNT', bancorConverter, user2, 'fund;TKNB') 
            )

            await expectNoError( 
                transfer(bntToken, '999.00000000 BNT', bancorConverter, user1, 'fund;BNTSYS') 
            )
            await expectNoError( 
                transfer('eosio.token', '990.0000 SYS', bancorConverter, user1, 'fund;BNTSYS') 
            )

            await expectNoError( 
                transfer(bntToken, '999.00000000 BNT', bancorConverter, user1, 'fund;RELAY') 
            )
            await expectNoError( 
                transfer('eosio.token', '990.0000 EOS', bancorConverter, user1, 'fund;RELAY') 
            )

            await expectNoError( 
                transfer(bntToken, '999.00000000 BNT', bancorConverter, user1, 'fund;RELAYB') 
            )
            await expectNoError( 
                transfer('eosio.token', '990.0000 EOS', bancorConverter, user1, 'fund;RELAYB') 
            )
            
            result = await getReserve('BNT', bancorConverter, 'RELAY')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].balance, '999.00000000 BNT', "withdrawal invalid - RELAY")

            result = await getReserve('EOS', bancorConverter, 'RELAY')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].balance, '990.0000 EOS', "funded amount invalid - RELAY")
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
    });
    describe('Converters Balances Management', async function() {
        it('ensures that converters balances sum is equal to the bancorConverter\'s total BNT balance [Load Test]', async function () {
            this.timeout(30000)
            const bntTxs = []
            const multiTxs = []
            for (let i = 0; i < 5; i++) {
                bntTxs.push(
                    transfer(bntToken, `${randomAmount({min: 8, max: 12, decimals: 8 })} BNT`, bancorNetwork, user1, 
                        `1,${bancorConverter}:TKNA TKNA,0.0001,${user1}`
                    ),
                    transfer(bntToken, `${randomAmount({min: 8, max: 12, decimals: 8 })} BNT`, bancorNetwork, user2, 
                        `1,${bancorConverter}:TKNB TKNB,0.0001,${user2}`
                    )
                )
                multiTxs.push(
                    transfer(multiToken, `${randomAmount({min: 1, max: 8, decimals: 4 })} TKNA`, bancorNetwork, user1, 
                        `1,${bancorConverter}:TKNA BNT,0.0000000001,${user1}`
                    ),
                    transfer(multiToken, `${randomAmount({min: 1, max: 8, decimals: 4 })} TKNB`, bancorNetwork, user2, 
                        `1,${bancorConverter}:TKNB BNT,0.0000000001,${user2}`
                    ),
                    transfer(multiToken, `${randomAmount({min: 1, max: 8, decimals: 4 })} TKNA`, bancorNetwork, user1, 
                        `1,${bancorConverter}:TKNA BNT ${bancorConverter}:TKNB TKNB,0.0001,${user1}`
                    ),
                    transfer(multiToken, `${randomAmount({min: 1, max: 8, decimals: 4 })} TKNB`, bancorNetwork, user2, 
                        `1,${bancorConverter}:TKNB BNT ${bancorConverter}:TKNA TKNA,0.0001,${user2}`
                    )
                )
            }
            await Promise.all(bntTxs)
            await Promise.all(multiTxs)

            const TokenAReserveBalance = Number((await getReserve('BNT', bancorConverter, 'TKNA')).rows[0].balance.split(' ')[0])
            const TokenBReserveBalance = Number((await getReserve('BNT', bancorConverter, 'TKNB')).rows[0].balance.split(' ')[0])
            const BNTEOSReserveBalance = Number((await getReserve('BNT', bancorConverter, 'BNTEOS')).rows[0].balance.split(' ')[0])
            const BNTSYSReserveBalance = Number((await getReserve('BNT', bancorConverter, 'BNTSYS')).rows[0].balance.split(' ')[0])
            const RELAYReserveBalance = Number((await getReserve('BNT', bancorConverter, 'RELAY')).rows[0].balance.split(' ')[0])
            const RELAYBReserveBalance = Number((await getReserve('BNT', bancorConverter, 'RELAYB')).rows[0].balance.split(' ')[0])
            const TESTReserveBalance = Number((await getReserve('BNT', bancorConverter, 'TEST')).rows[0].balance.split(' ')[0])
            const INACTIVReserveBalance = Number((await getReserve('BNT', bancorConverter, 'INACTIV')).rows[0].balance.split(' ')[0])
            
            const totalBntBalance = (await getBalance(bancorConverter, bntToken, 'BNT')).rows[0].balance.split(' ')[0]
            
            const reserveBalancesSum = [TokenAReserveBalance, TokenBReserveBalance, BNTEOSReserveBalance, BNTSYSReserveBalance, RELAYReserveBalance, RELAYBReserveBalance, TESTReserveBalance, INACTIVReserveBalance]
                .reduce((sum, c) => sum.add(c), Decimal(0));
            assert.equal(reserveBalancesSum.toFixed(8), totalBntBalance)
        })
        it('ensures that fund and withdraw are working properly (pre-launch)', async () => {
            const bntReserveBefore = (await getReserve('BNT', bancorConverter, 'BNTEOS')).rows[0].balance

            let result = await getBalance(user1, bntToken, 'BNT')
            await expectNoError(
                transfer(bntToken, '10.00000000 BNT', bancorConverter, user1, 'fund;BNTEOS'),
            )
            result = await getReserve('BNT', bancorConverter, 'BNTEOS')
            let bntReserveAfter = result.rows[0].balance
            assert.equal(bntReserveBefore, bntReserveAfter, 'BNT wasnt supposed to change yet - BNTEOS')

            const accountBalance = parseFloat((await getAccount(user1, 'BNTEOS', 'BNT')).rows[0].quantity.split(' ')[0])
            await expectError(
                withdraw(user1, `${(accountBalance + 0.00000001).toFixed(8)} BNT`, 'BNTEOS'),
                "insufficient balance"
            ) 
            await expectNoError(
                withdraw(user1, `${(accountBalance - 0.00000001).toFixed(8)} BNT`, 'BNTEOS'),
            ) 
            
            result = await getAccount(user1, 'BNTEOS', 'BNT')
            let balance = result.rows[0].quantity.split(' ')[0]
            assert.equal(balance, '0.00000001', 'BNT wasnt supposed to change yet - BNTEOS')

            await expectNoError(
                transfer(bntToken, '9.99999999 BNT', bancorConverter, user1, 'fund;BNTEOS'),
            )
            result = await getAccount(user1, 'BNTEOS', 'BNT')
            balance = result.rows[0].quantity.split(' ')[0]
            assert.equal(balance, '10.00000000')

            await expectNoError(
                transfer('eosio.token', '10.0000 EOS', bancorConverter, user1, 'fund;BNTEOS'),
            )
            result = await getAccount(user1, 'BNTEOS', 'EOS')
            balance = result.rows[0].quantity.split(' ')[0]
            assert.equal(balance, '10.0000')
        })
        it('ensures that fund and liquidate are working properly (post-launch)', async () => {
            let result = await getBalance(user1, multiToken, 'BNTEOS')
            assert.equal(result.rows.length, 1)
            let userSmartTokenBalanceBefore = parseFloat(result.rows[0].balance)
            await expectError( // insufficient balance in account row
                fund(user1, '2000.0000 BNTEOS'),
                "insufficient balance"
            )
            const eosReserveBalanceBefore = (await getReserve('EOS', bancorConverter, 'BNTEOS')).rows[0].balance.split(' ')[0]
            const bntReserveBalanceBefore = (await getReserve('BNT', bancorConverter, 'BNTEOS')).rows[0].balance.split(' ')[0]
            
            let eosAccountBalances = await getAccount(user1, "BNTEOS", "EOS")
            let bntAccountBalances = await getAccount(user1, "BNTEOS", "BNT")
            const eosAccountBalanceBefore = eosAccountBalances.rows.length > 0 ? eosAccountBalances.rows[0].quantity.split(' ')[0] : 0;
            const bntAccountBalanceBefore = bntAccountBalances.rows.length > 0 ? bntAccountBalances.rows[0].quantity.split(' ')[0] : 0;
            await expectNoError(
                fund(user1, '100.0000 BNTEOS')
            )
                
            const eosReserveBalanceAfter = (await getReserve('EOS', bancorConverter, 'BNTEOS')).rows[0].balance.split(' ')[0]
            const bntReserveBalanceAfter = (await getReserve('BNT', bancorConverter, 'BNTEOS')).rows[0].balance.split(' ')[0]
            
            eosAccountBalances = await getAccount(user1, "BNTEOS", "EOS")
            bntAccountBalances = await getAccount(user1, "BNTEOS", "BNT")
            const eosAccountBalanceAfter = eosAccountBalances.rows.length > 0 ? eosAccountBalances.rows[0].quantity.split(' ')[0] : 0;
            const bntAccountBalanceAfter = bntAccountBalances.rows.length > 0 ? bntAccountBalances.rows[0].quantity.split(' ')[0] : 0;
            
            
            const eosReserveBalanceDelta = Math.abs(Decimal(eosReserveBalanceAfter).sub(eosReserveBalanceBefore).toFixed(4))
            const bntReserveBalanceDelta = Math.abs(Decimal(bntReserveBalanceAfter).sub(bntReserveBalanceBefore).toFixed(8))
            const eosAccountDelta = Math.abs(Decimal(eosAccountBalanceAfter).sub(eosAccountBalanceBefore).toFixed(4))
            const bntAccountDelta = Math.abs(Decimal(bntAccountBalanceAfter).sub(bntAccountBalanceBefore).toFixed(8))
            
            assert.equal(eosReserveBalanceDelta, eosAccountDelta)
            assert.equal(bntReserveBalanceDelta, bntAccountDelta)
            
            // Fetch users balance and confirm he was awarded the 10000 BNT he asked for
            result = await getBalance(user1, multiToken, 'BNTEOS')
            assert.equal(result.rows.length, 1)
            let userSmartTokenBalanceAfter = parseFloat(result.rows[0].balance)
            let delta = (userSmartTokenBalanceAfter - userSmartTokenBalanceBefore).toFixed(4)
            assert.equal(delta, '100.0000', 'incorrect BNTEOS issued')

            await expectNoError( 
                transfer(multiToken, '100.0000 BNTEOS', bancorConverter, user1, 'liquidate')
            )
            
            const eosReserveBalanceFinal = (await getReserve('EOS', bancorConverter, 'BNTEOS')).rows[0].balance.split(' ')[0]
            const bntReserveBalanceFinal = (await getReserve('BNT', bancorConverter, 'BNTEOS')).rows[0].balance.split(' ')[0]
            
            assert.isAtMost(Math.abs(Decimal(eosReserveBalanceFinal).sub(eosReserveBalanceFinal).toFixed(4)), 0.0001)
            assert.isAtMost(Math.abs(Decimal(bntReserveBalanceFinal).sub(bntReserveBalanceBefore).toFixed(8)), 0.00000001)

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
                setMaxfee(bancorConverter, 20000)
            )
            result = await getSettings(bancorConverter)
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].max_fee, 20000, "max fee not set correctly - bancorConverter")
        })
        it("should throw error when trying to call 'setreserve' without proper permissions", async () => {
            const actor = user2
            await expectError(
                setreserve(true, bntToken, 'BNT', bancorConverter, 'TKNA', actor, 200000),
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
        it("trying to delete BNT reserve when it's not empty - should throw", async () => { 
            await expectError(
                delreserve('BNT', user1, bancorConverter, 'TKNA'), 
                "a reserve can only be deleted if it's converter is inactive"
            )
        })
        it("trying to delete TKNA converter when reserves not empty - should throw", async () => { 
            await expectError(
                delConverter('TKNA', user1), 
                "delete reserves first"
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
        it("should throw an error when trying to update an existing reserve", async () => {
            const actor = user1
            await expectError(
                setreserve(true, bntToken, 'BNT', bancorConverter, 'TKNA', actor, 200000),
                'reserve already exists'
            )
        })
    })
    describe('Formula', async () => {
        it('[Fund] ensures a proper funding cost calculation (total ratio < 100%)', async () => {
            const tokenSymbol = 'TKNA';
            
            let accountReserveBalances = (await getAccount(user1, tokenSymbol, 'BNT')).rows;
            const accountReserveBalanceBefore = accountReserveBalances.length > 0 ? accountReserveBalances[0].quantity.split(' ')[0] : 0;

            const fundingAmount = randomAmount({min: 1, max: 2, decimals: 4 });
            const supply = (await get(multiToken, tokenSymbol)).rows[0].supply.split(' ')[0]
            const { ratio, balance} = (await getReserve('BNT', bancorConverter, tokenSymbol)).rows[0]
            const reserveBalance = Number(balance.split(' ')[0])
            const fundCost = calculateFundCost(fundingAmount, supply, reserveBalance, ratio);
            
            await expectNoError( 
                transfer(bntToken, `${toFixedRoundUp(fundCost, 8)} BNT`, bancorConverter, user1, `fund;${tokenSymbol}`) 
            );
            await expectNoError(
                fund(user1, `${fundingAmount} ${tokenSymbol}`)
            )

            accountReserveBalances = (await getAccount(user1, tokenSymbol, 'BNT')).rows;
            const accountReserveBalanceAfter = accountReserveBalances.length > 0 ? accountReserveBalances[0].quantity.split(' ')[0] : 0;
            
            assert.equal(accountReserveBalanceBefore, accountReserveBalanceAfter);            
        });
        it('[Fund] ensures a proper funding cost calculation (total ratio == 100%)', async () => {
            const tokenSymbol = 'RELAY';
            
            let accountReserveABalance = (await getAccount(user1, tokenSymbol, 'BNT')).rows;
            let accountReserveBBalance = (await getAccount(user1, tokenSymbol, 'EOS')).rows;
            const accountReserveABalanceBefore = accountReserveABalance.length > 0 ? accountReserveABalance[0].quantity.split(' ')[0] : 0;
            const accountReserveBBalanceBefore = accountReserveBBalance.length > 0 ? accountReserveBBalance[0].quantity.split(' ')[0] : 0;

            const fundingAmount = randomAmount({min: 1, max: 2, decimals: 4 });
            const supply = (await get(multiToken, tokenSymbol)).rows[0].supply.split(' ')[0]
            const reserveA = (await getReserve('BNT', bancorConverter, tokenSymbol)).rows[0]
            const reserveB = (await getReserve('EOS', bancorConverter, tokenSymbol)).rows[0]
            const reserveABalance = Number(reserveA.balance.split(' ')[0])
            const reserveBBalance = Number(reserveB.balance.split(' ')[0])
            const totalRatio = reserveA.ratio + reserveB.ratio;
            
            const reserveAFundCost = calculateFundCost(fundingAmount, supply, reserveABalance, totalRatio);
            const reserveBFundCost = calculateFundCost(fundingAmount, supply, reserveBBalance, totalRatio);
            await expectNoError( 
                transfer(bntToken, `${toFixedRoundUp(reserveAFundCost, 8)} BNT`, bancorConverter, user1, `fund;${tokenSymbol}`) 
            );
            await expectNoError( 
                transfer('eosio.token', `${toFixedRoundUp(reserveBFundCost, 4)} EOS`, bancorConverter, user1, `fund;${tokenSymbol}`) 
            );
            await expectNoError(
                fund(user1, `${fundingAmount} ${tokenSymbol}`)
            )

            accountReserveABalance = (await getAccount(user1, tokenSymbol, 'BNT')).rows;
            accountReserveBBalance = (await getAccount(user1, tokenSymbol, 'EOS')).rows;
            const accountReserveABalanceAfter = accountReserveABalance.length > 0 ? accountReserveABalance[0].quantity.split(' ')[0] : 0;
            const accountReserveBBalanceAfter = accountReserveBBalance.length > 0 ? accountReserveBBalance[0].quantity.split(' ')[0] : 0;

            assert.equal(accountReserveABalanceBefore, accountReserveABalanceAfter);
            assert.equal(accountReserveBBalanceBefore, accountReserveBBalanceAfter);        
        });
        it('[Liquidate] ensures a proper liquidation return calculation (total ratio < 100%)', async () => {
            const tokenSymbol = 'TKNA';
            const accountReserveBalanceBefore = Number((await getBalance(user1, bntToken, 'BNT')).rows[0].balance.split(' ')[0])
            
            const { ratio, balance} = (await getReserve('BNT', bancorConverter, tokenSymbol)).rows[0]
            const reserveBalance = Number(balance.split(' ')[0])
            
            const supply = (await get(multiToken, tokenSymbol)).rows[0].supply.split(' ')[0]
            const liquidationAmount = randomAmount({min: 1, max: Math.ceil(supply / 2), decimals: 4 })

            const expectedLiquidationReturn = calculateLiquidateReturn(liquidationAmount, supply, reserveBalance, ratio)

            await expectNoError(
                transfer(multiToken, `${liquidationAmount} ${tokenSymbol}`, bancorConverter, user1, 'liquidate')
            )

            const accountReserveBalanceAfter = Number((await getBalance(user1, bntToken, 'BNT')).rows[0].balance.split(' ')[0])
            
            assert.equal((accountReserveBalanceAfter - accountReserveBalanceBefore).toFixed(8), toFixedRoundDown(expectedLiquidationReturn, 8));   
        });
        it('[Liquidate] ensures a proper liquidation return calculation (total ratio == 100%)', async () => {
            const tokenSymbol = 'RELAY';
            const accountReserveABalanceBefore = Number((await getBalance(user1, bntToken, 'BNT')).rows[0].balance.split(' ')[0])
            const accountReserveBBalanceBefore = Number((await getBalance(user1, 'eosio.token', 'EOS')).rows[0].balance.split(' ')[0])
            
            const reserveA = (await getReserve('BNT', bancorConverter, tokenSymbol)).rows[0]
            const reserveB = (await getReserve('EOS', bancorConverter, tokenSymbol)).rows[0]
            const reserveABalance = Number(reserveA.balance.split(' ')[0])
            const reserveBBalance = Number(reserveB.balance.split(' ')[0])
            const totalRatio = reserveA.ratio + reserveB.ratio
            
            const supply = (await get(multiToken, tokenSymbol)).rows[0].supply.split(' ')[0]
            const liquidationAmount = randomAmount({min: 1, max: Math.ceil(supply / 2), decimals: 4 })

            const expectedReserveALiquidationReturn = calculateLiquidateReturn(liquidationAmount, supply, reserveABalance, totalRatio)
            const expectedReserveBLiquidationReturn = calculateLiquidateReturn(liquidationAmount, supply, reserveBBalance, totalRatio)

            await expectNoError(
                transfer(multiToken, `${liquidationAmount} ${tokenSymbol}`, bancorConverter, user1, 'liquidate')
            )

            const accountReserveABalanceAfter = Number((await getBalance(user1, bntToken, 'BNT')).rows[0].balance.split(' ')[0])
            const accountReserveBBalanceAfter = Number((await getBalance(user1, 'eosio.token', 'EOS')).rows[0].balance.split(' ')[0])
            
            assert.equal((accountReserveABalanceAfter - accountReserveABalanceBefore).toFixed(8), toFixedRoundDown(expectedReserveALiquidationReturn, 8));
            assert.equal((accountReserveBBalanceAfter - accountReserveBBalanceBefore).toFixed(4), toFixedRoundDown(expectedReserveBLiquidationReturn, 4));
        });
        it('ensures a proper return amount calculation (no fee) [RESERVE --> SMART]', async () => {
            const inputAmount = '2.20130604'
            
            const initialBalance = Number((await getBalance(user1, multiToken, 'TKNA')).rows[0].balance.split(' ')[0])
            const tokenStats = (await get(multiToken, 'TKNA')).rows[0]
            const reserveData = (await getReserve('BNT', bancorConverter, 'TKNA')).rows[0]

            const supply = tokenStats.supply.split(' ')[0]
            const reserveBalance = Number(reserveData.balance.split(' ')[0])
            const { ratio } = reserveData

            const events = await extractEvents(
                convertBNT(inputAmount, 'TKNA', `${bancorConverter}:TKNA`)
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
            
            const fromTokenReserveData = (await getReserve('BNT', bancorConverter, 'RELAY')).rows[0]
            const toTokenReserveData = (await getReserve('EOS', bancorConverter, 'RELAY')).rows[0]
            const fromTokenReserveBalance = Number(fromTokenReserveData.balance.split(' ')[0])
            const toTokenReserveBalance = Number(toTokenReserveData.balance.split(' ')[0])
            
            const events = await extractEvents(
                convertBNT(inputAmount, 'EOS', `${bancorConverter}:RELAY`)
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
            
            const fromTokenReserveData = (await getReserve('BNT', bancorConverter, 'RELAY')).rows[0]
            const toTokenReserveData = (await getReserve('EOS', bancorConverter, 'RELAY')).rows[0]
            const fromTokenReserveBalance = Number(fromTokenReserveData.balance.split(' ')[0])
            const toTokenReserveBalance = Number(toTokenReserveData.balance.split(' ')[0])
            
            const events = await extractEvents(
                convertBNT(inputAmount, 'EOS', `${bancorConverter}:RELAY`)
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
            const reserveData = (await getReserve('BNT', bancorConverter, 'TKNA')).rows[0]
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
            const reserveData = (await getReserve('BNT', bancorConverter, 'TKNB')).rows[0]

            const supply = tokenStats.supply.split(' ')[0]
            const reserveBalance = Number(reserveData.balance.split(' ')[0])
            const { ratio } = reserveData

            const events = await extractEvents(
                convertBNT(inputAmount, 'TKNB', `${bancorConverter}:TKNB`, user1, user1)
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
            const reserveData = (await getReserve('BNT', bancorConverter, 'TKNB')).rows[0]
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
            
            const fromTokenReserveData = (await getReserve('EOS', bancorConverter, 'RELAY')).rows[0]
            const toTokenReserveData = (await getReserve('BNT', bancorConverter, 'RELAY')).rows[0]
            const fromTokenReserveBalance = Number(fromTokenReserveData.balance.split(' ')[0])
            const toTokenReserveBalance = Number(toTokenReserveData.balance.split(' ')[0])
            
            const tokenStats = (await get(multiToken, 'TKNA')).rows[0]
            const reserveData = (await getReserve('BNT', bancorConverter, 'TKNA')).rows[0]

            const secondHopSupply = tokenStats.supply.split(' ')[0]
            const secondHopReserveBalance = Number(reserveData.balance.split(' ')[0])
            const secondHopRatio = reserveData.ratio


            const events = await extractEvents(
                convertTwice(inputAmount, 'eosio.token', 'EOS', 'TKNA', `${bancorConverter}:RELAY`, `${bancorConverter}:TKNA`)
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
            const reserveData = (await getReserve('BNT', bancorConverter, 'TKNA')).rows[0]

            const firstHopSupply = tokenStats.supply.split(' ')[0]
            const firstHopReserveBalance = Number(reserveData.balance.split(' ')[0])
            const firstHopRatio = reserveData.ratio

            const secondHopFromTokenReserveData = (await getReserve('BNT', bancorConverter, 'RELAY')).rows[0]
            const secondHopToTokenReserveData = (await getReserve('EOS', bancorConverter, 'RELAY')).rows[0]
            const secondHopFromTokenReserveBalance = Number(secondHopFromTokenReserveData.balance.split(' ')[0])
            const secondHopToTokenReserveBalance = Number(secondHopToTokenReserveData.balance.split(' ')[0])

            const events = await extractEvents(
                convertTwice(inputAmount, multiToken, 'TKNA', 'EOS', `${bancorConverter}:TKNA`, `${bancorConverter}:RELAY`)
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
            
            const firstHopFromTokenReserveData = (await getReserve('EOS', bancorConverter, 'RELAY')).rows[0]
            const firstHopToTokenReserveData = (await getReserve('BNT', bancorConverter, 'RELAY')).rows[0]
            const firstHopFromTokenReserveBalance = Number(firstHopFromTokenReserveData.balance.split(' ')[0])
            const firstHopToTokenReserveBalance = Number(firstHopToTokenReserveData.balance.split(' ')[0])
            
            const secondHopFromTokenReserveData = (await getReserve('BNT', bancorConverter, 'RELAYB')).rows[0]
            const secondHopToTokenReserveData = (await getReserve('EOS', bancorConverter, 'RELAYB')).rows[0]
            const secondHopFromTokenReserveBalance = Number(secondHopFromTokenReserveData.balance.split(' ')[0])
            const secondHopToTokenReserveBalance = Number(secondHopToTokenReserveData.balance.split(' ')[0])

            const events = await extractEvents(
                convertTwice(inputAmount, 'eosio.token', 'EOS', 'EOS', `${bancorConverter}:RELAY`, `${bancorConverter}:RELAYB`)
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
        it('[liquidate] make sure that liquidating the entire pool token supply completely empties the reserves', async () => {
            const converterCurrencySymbol = 'RELAY';
            const totalSupply = (await get(multiToken, converterCurrencySymbol)).rows[0].supply
            await expectNoError( 
                transfer(multiToken, totalSupply, bancorConverter, user1, 'liquidate')
            )
            for (const reserveSymbol of ['BNT', 'EOS']) {
                const { balance } = (await getReserve(reserveSymbol, bancorConverter, converterCurrencySymbol)).rows[0]
                assert.equal(Number(balance.split(' ')[0]), 0, 'reserve is not empty');
            }
        });

    })
    describe('Events', async () => {
        it('[PriceDataEvents: reserve --> smart] 1 hop conversion', async function() {
            const amount = randomAmount({min: 1, max: 5, decimals: 8 });
            let res = await getReserve('BNT', bancorConverter, 'BNTEOS');
            const initialFromTokenReserveBalance = parseFloat(res.rows[0].balance.split(' ')[0])
    
            const { price_data: [fromTokenPriceDataEvent] } = await extractEvents(
                convertBNT(amount)
            ) //to BNTEOS
    
            assert.equal(fromTokenPriceDataEvent.reserve_ratio, 500000, "unexpected reserve_ratio");
            
            const expectedFromTokenReserveBalance = parseFloat(amount) + initialFromTokenReserveBalance;
            assert.equal(parseFloat(fromTokenPriceDataEvent.reserve_balance).toFixed(8), expectedFromTokenReserveBalance.toFixed(8), "unexpected reserve_balance");
        
            res = await get(multiToken, 'BNTEOS');
            const expectedSmartSupply = parseFloat(res.rows[0].supply.split(' ')[0])
            assert.equal(expectedSmartSupply, parseFloat(fromTokenPriceDataEvent.smart_supply).toFixed(4), 'unexpected smart supply');
        });
        it('[PriceDataEvents: smart --> reserve] 1 hop conversion', async function() {
            const amount = randomAmount({min: 1, max: 5, decimals: 4 });
                
            let res = await getReserve('BNT', bancorConverter, 'BNTEOS');
            const initialFromTokenReserveBalance = parseFloat(res.rows[0].balance.split(' ')[0])
            const { conversion: [conversionEvent], price_data: [fromTokenPriceDataEvent] } = await extractEvents(
                convert(`${amount} BNTEOS`, multiToken, [`${bancorConverter}:BNTEOS BNT`])
            )
    
            assert.equal(fromTokenPriceDataEvent.reserve_ratio, 500000, "unexpected reserve_ratio");
            const expectedFromTokenReserveBalance = Decimal(initialFromTokenReserveBalance).sub(conversionEvent.return).toFixed(8);
            assert.equal(parseFloat(fromTokenPriceDataEvent.reserve_balance), expectedFromTokenReserveBalance, "unexpected reserve_balance");
        
            res = await get(multiToken, 'BNTEOS');
            const expectedSmartSupply = parseFloat(res.rows[0].supply.split(' ')[0])
            assert.equal(expectedSmartSupply, parseFloat(fromTokenPriceDataEvent.smart_supply).toFixed(4), 'unexpected smart supply');
        });
    });

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
        it('[liquidate] ensures assets from an unknown contract are rejected', async () => {
            await expectError( 
                transfer('bnt2sysrelay',  '1.00000000 BNTSYS', bancorConverter ,user1, 'liquidate'),
                'bad origin for this transfer'
            )
        });
    });
})
