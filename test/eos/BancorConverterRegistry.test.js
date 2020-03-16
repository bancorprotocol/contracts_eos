const assert = require('chai').assert
const Decimal = require('decimal.js')
const path = require('path')
const config = require('../../config/accountNames.json')
const {  
    expectError, 
    expectNoError,
    randomAmount,
    newAccount,
    setCode,
    getTableRows
} = require('./common/utils')

const {
    getBalance,
    convertBNT,
    convertTwice,
    convertMulti,
    convert
} = require('./common/token')

const {
    addConverter,
    rmConverter
} = require('./common/bancor-converter-registry')

const { 
    getReserve
} = require('./common/converter')

const { ERRORS } = require('./common/errors')
const user1 = config.MASTER_ACCOUNT
const user2 = config.TEST_ACCOUNT
const bancorConverter = config.MULTI_CONVERTER_ACCOUNT
const multiToken = config.MULTI_TOKEN_ACCOUNT
const bntToken = config.BNT_TOKEN_ACCOUNT

const CONTRACT_NAME = 'BancorConverterRegistry'
const CODE_FILE_PATH = path.join(__dirname, "../../contracts/eos/", CONTRACT_NAME, `${CONTRACT_NAME}.wasm`)
const ABI_FILE_PATH = path.join(__dirname, "../../contracts/eos/", CONTRACT_NAME, `${CONTRACT_NAME}.abi`)

describe.only(CONTRACT_NAME, () => {
    let converterRegistryAccount;

    before(async () => {
        converterRegistryAccount = (await newAccount()).accountName;
        await setCode(converterRegistryAccount, CODE_FILE_PATH, ABI_FILE_PATH);
    });
    describe('end to end converter registration and deletion', async () => {
        const poolTokenExtendedSymbol = { sym: '4,BNTEOS', contract: multiToken }
        const poolTokenSymbol = poolTokenExtendedSymbol.sym.split(',')[1]
        const reservesExtendedSymbols = [
            { sym: '8,BNT', contract: config.BNT_TOKEN_ACCOUNT },
            { sym: '4,EOS', contract: 'eosio.token' }
        ]
        it("[addconverter] verifies data is registered properly", async () => {
            await expectNoError(
                addConverter(converterRegistryAccount, bancorConverter, poolTokenSymbol)
            )
            
            const { rows: [registeredConverterData] } = await getTableRows(converterRegistryAccount, poolTokenSymbol, 'converters', bancorConverter)
            assert.equal(registeredConverterData.contract, bancorConverter)
            assert.equal(registeredConverterData.pool_token.sym, `4,${poolTokenSymbol}`)
            assert.equal(registeredConverterData.pool_token.contract, poolTokenExtendedSymbol.contract)

            const { rows: [registeredLiquidityPoolData] } = await getTableRows(converterRegistryAccount, poolTokenSymbol, 'liqdtypools', bancorConverter)
            assert.deepEqual(registeredLiquidityPoolData, registeredConverterData)
            
            const { rows: [registeredPoolTokenData] } = await getTableRows(converterRegistryAccount, poolTokenSymbol, 'pooltokens', bancorConverter)
            assert.equal(registeredPoolTokenData.token.sym, `4,${poolTokenSymbol}`)
            assert.equal(registeredPoolTokenData.token.contract, poolTokenExtendedSymbol.contract)

            let remainingReserves = reservesExtendedSymbols
            const { rows: registeredPoolTokenConvertiblePairsData } = await getTableRows(converterRegistryAccount, poolTokenSymbol, 'cnvrtblpairs', bancorConverter)
            for (const convertiblePair of registeredPoolTokenConvertiblePairsData) {
                assert.deepEqual(convertiblePair.from_token, registeredPoolTokenData.token)
                assert.deepEqual(convertiblePair.converter, registeredConverterData)

                const reserveIndex = remainingReserves.findIndex(reserve => (reserve.sym === convertiblePair.to_token.sym && reserve.contract === convertiblePair.to_token.contract))
                assert.notEqual(reserveIndex, -1)
                remainingReserves = remainingReserves.slice(0, reserveIndex).concat(remainingReserves.slice(reserveIndex + 1))
            }
            
            for (const reserve of reservesExtendedSymbols) {
                const reserveSymbolCode = reserve.sym.split(',')[1]
                const { rows: registeredReserveConvertiblePairsData } = await getTableRows(converterRegistryAccount, reserveSymbolCode, 'cnvrtblpairs', bancorConverter)
                
                let possibleToTokens = [
                    reservesExtendedSymbols.find(({ sym }) => sym !== reserve.sym),
                    poolTokenExtendedSymbol
                ]
                for (const convertiblePair of registeredReserveConvertiblePairsData) {
                    assert.deepEqual(convertiblePair.from_token, reserve)
                    assert.deepEqual(convertiblePair.converter, registeredConverterData)

                    const toTokenIndex = possibleToTokens.findIndex(toToken => (toToken.sym === convertiblePair.to_token.sym && toToken.contract === convertiblePair.to_token.contract))
                    assert.notEqual(toTokenIndex, -1)
                    possibleToTokens = possibleToTokens.slice(0, toTokenIndex).concat(possibleToTokens.slice(toTokenIndex + 1))
                }
            }
        })
        it('[rmconverter] verifies all registered data gets deleted', async () => {
            await expectNoError(
                rmConverter(converterRegistryAccount, bancorConverter, poolTokenSymbol, { actor: converterRegistryAccount, permission: 'active' })
            )
            const { rows: [registeredConverterData] } = await getTableRows(converterRegistryAccount, poolTokenSymbol, 'converters', bancorConverter)
            assert.isUndefined(registeredConverterData)

            const { rows: [registeredLiquidityPoolData] } = await getTableRows(converterRegistryAccount, poolTokenSymbol, 'liqdtypools', bancorConverter)
            assert.isUndefined(registeredLiquidityPoolData)
            
            const { rows: [registeredPoolTokenData] } = await getTableRows(converterRegistryAccount, poolTokenSymbol, 'pooltokens', bancorConverter)
            assert.isUndefined(registeredPoolTokenData)

            const { rows: registeredPoolTokenConvertiblePairsData } = await getTableRows(converterRegistryAccount, poolTokenSymbol, 'cnvrtblpairs', bancorConverter)
            assert.lengthOf(registeredPoolTokenConvertiblePairsData, 0)
            
            for (const reserve of reservesExtendedSymbols) {
                const reserveSymbolCode = reserve.sym.split(',')[1]
                const { rows: registeredReserveConvertiblePairsData } = await getTableRows(converterRegistryAccount, reserveSymbolCode, 'cnvrtblpairs', bancorConverter)
                
                assert.lengthOf(registeredReserveConvertiblePairsData, 0)
            }
        })
    });
})
