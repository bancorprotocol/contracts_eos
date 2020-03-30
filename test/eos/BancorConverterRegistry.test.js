const assert = require('chai').assert
const path = require('path')
const config = require('../../config/accountNames.json')
const {  
    expectError, 
    expectNoError,
    newAccount,
    setCode
} = require('./common/utils')

const {
    issue,
    transfer,
    getBalance,
    convertBNT,
    convertTwice,
    convertMulti,
    convert
} = require('./common/token')

const {
    addConverter,
    rmConverter,
    getConverter,
    getLiquidityPool,
    getPoolToken,
    getConvertiblePairs
} = require('./common/bancor-converter-registry')

const { 
    getReserve
} = require('./common/converter')

const { ERRORS } = require('./common/errors')
const bancorConverter = config.BANCOR_CONVERTER_ACCOUNT
const multiToken = config.MULTI_TOKEN_ACCOUNT

const CONTRACT_NAME = 'BancorConverterRegistry'
const CODE_FILE_PATH = path.join(__dirname, "../../contracts/eos/", CONTRACT_NAME, `${CONTRACT_NAME}.wasm`)
const ABI_FILE_PATH = path.join(__dirname, "../../contracts/eos/", CONTRACT_NAME, `${CONTRACT_NAME}.abi`)

describe.only(CONTRACT_NAME, () => {
    let converterRegistryAccount;

    before(async () => {
        converterRegistryAccount = (await newAccount()).accountName;
        console.info(`Deployed @ ${converterRegistryAccount}`)
        await setCode(converterRegistryAccount, CODE_FILE_PATH, ABI_FILE_PATH);
    });
    describe('inactive converter logic', async () => {
        const inactivePoolToken = 'INACTIV'
        it('[addconverter] ensures an error is thrown when adding an inactive converter', async () => {
            await expectError(
                addConverter(converterRegistryAccount, bancorConverter, inactivePoolToken),
                ERRORS.INACTIVE_CONVERTER
            )
        });
    });
    describe('end to end converter registration and deletion', async () => {
        const converters = [
            {
                poolToken: { sym: '4,BNTEOS', contract: multiToken },
                reserves: [
                    { sym: '8,BNT', contract: config.BNT_TOKEN_ACCOUNT },
                    { sym: '4,EOS', contract: 'eosio.token' }
                ]
            },
            {
                poolToken: { sym: '4,TEST', contract: multiToken },
                reserves: [
                    { sym: '8,BNT', contract: config.BNT_TOKEN_ACCOUNT },
                    { sym: '4,EOS', contract: 'eosio.token' }
                ]
            }

        ]
        it("[addconverter] verifies data is registered properly", async () => {
            for (const { poolToken, reserves } of converters) {
                const poolTokenSymbol = poolToken.sym.split(',')[1]

                await expectNoError(
                    addConverter(converterRegistryAccount, bancorConverter, poolTokenSymbol)
                )
                
                const { rows: [registeredConverterData] } = await getConverter(converterRegistryAccount, bancorConverter, poolTokenSymbol);
                assert.equal(registeredConverterData.converter.contract, bancorConverter)
                assert.equal(registeredConverterData.converter.pool_token.sym, `4,${poolTokenSymbol}`)
                assert.equal(registeredConverterData.converter.pool_token.contract, poolToken.contract)
    
                const { rows: [registeredLiquidityPoolData] } = await getLiquidityPool(converterRegistryAccount, bancorConverter, poolTokenSymbol);
                assert.deepEqual(registeredLiquidityPoolData.converter, registeredConverterData.converter)
                
                const { rows: [registeredPoolTokenData] } = await getPoolToken(converterRegistryAccount, poolToken.contract, poolTokenSymbol)
                assert.equal(registeredPoolTokenData.token.sym, `4,${poolTokenSymbol}`)
                assert.equal(registeredPoolTokenData.token.contract, poolToken.contract)
    
                let remainingReserves = reserves
                
                const { rows: registeredPoolTokenConvertiblePairsData } = await getConvertiblePairs(converterRegistryAccount, poolTokenSymbol, bancorConverter, poolTokenSymbol)
                for (const convertiblePair of registeredPoolTokenConvertiblePairsData) {
                    assert.deepEqual(convertiblePair.from_token, registeredPoolTokenData.token)
                    assert.deepEqual(convertiblePair.converter, registeredConverterData.converter)
    
                    const reserveIndex = remainingReserves.findIndex(reserve => (reserve.sym === convertiblePair.to_token.sym && reserve.contract === convertiblePair.to_token.contract))
                    assert.notEqual(reserveIndex, -1)
                    remainingReserves = remainingReserves.slice(0, reserveIndex).concat(remainingReserves.slice(reserveIndex + 1))
                }
                
                for (const reserve of reserves) {
                    const reserveSymbolCode = reserve.sym.split(',')[1]
                    const { rows: registeredReserveConvertiblePairsData } = await getConvertiblePairs(converterRegistryAccount, reserveSymbolCode, bancorConverter, poolTokenSymbol)
                    
                    let possibleToTokens = [
                        reserves.find(({ sym }) => sym !== reserve.sym),
                        poolToken
                    ]
                    for (const convertiblePair of registeredReserveConvertiblePairsData) {
                        assert.deepEqual(convertiblePair.from_token, reserve)
                        assert.deepEqual(convertiblePair.converter, registeredConverterData.converter)
    
                        const toTokenIndex = possibleToTokens.findIndex(toToken => (toToken.sym === convertiblePair.to_token.sym && toToken.contract === convertiblePair.to_token.contract))
                        assert.notEqual(toTokenIndex, -1)
                        possibleToTokens = possibleToTokens.slice(0, toTokenIndex).concat(possibleToTokens.slice(toTokenIndex + 1))
                    }
                }
            }
        })
        it('[rmconverter] verifies all registered data gets deleted', async () => {
            for (const { poolToken, reserves } of converters) {
                const poolTokenSymbol = poolToken.sym.split(',')[1]
                
                await expectNoError(
                    rmConverter(converterRegistryAccount, bancorConverter, poolTokenSymbol, { actor: converterRegistryAccount, permission: 'active' })
                )
                const { rows: [registeredConverterData] } = await getConverter(converterRegistryAccount, bancorConverter, poolTokenSymbol);
                assert.isUndefined(registeredConverterData)
                
                const { rows: [registeredLiquidityPoolData] } = await getLiquidityPool(converterRegistryAccount, bancorConverter, poolTokenSymbol);
                assert.isUndefined(registeredLiquidityPoolData)
                
                const { rows: [registeredPoolTokenData] } = await getPoolToken(converterRegistryAccount, poolToken.contract, poolTokenSymbol)
                assert.isUndefined(registeredPoolTokenData)

                const { rows: registeredPoolTokenConvertiblePairsData } = await getConvertiblePairs(converterRegistryAccount, poolTokenSymbol, bancorConverter, poolTokenSymbol)
                assert.lengthOf(registeredPoolTokenConvertiblePairsData, 0)
                
                for (const reserve of reserves) {
                    const reserveSymbolCode = reserve.sym.split(',')[1]
                    const { rows: registeredReserveConvertiblePairsData } = await getConvertiblePairs(converterRegistryAccount, reserveSymbolCode, bancorConverter, poolTokenSymbol)
                    
                    assert.lengthOf(registeredReserveConvertiblePairsData, 0)
                }
            }
        })
    });
})
