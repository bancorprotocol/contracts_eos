const assert = require('chai').assert
const Decimal = require('decimal.js')
const path = require('path')
const config = require('../../config/accountNames.json')
const {  
    expectError, 
    expectNoError,
    randomAmount,
    newAccount,
    setCode
} = require('./common/utils')

const {
    getBalance,
    convertBNT,
    convertTwice,
    convertMulti,
    convert
} = require('./common/token')

const {
    addConverter
} = require('./common/converter-registry')

const { 
    getReserve
} = require('./common/converter')

const { ERRORS } = require('./common/errors')
const user1 = config.MASTER_ACCOUNT
const user2 = config.TEST_ACCOUNT
const bancorConverter = config.MULTI_CONVERTER_ACCOUNT
const bntToken = config.BNT_TOKEN_ACCOUNT

const CONTRACT_NAME = 'ConverterRegistry'
const CODE_FILE_PATH = path.join(__dirname, "../../contracts/eos/", CONTRACT_NAME, `${CONTRACT_NAME}.wasm`)
const ABI_FILE_PATH = path.join(__dirname, "../../contracts/eos/", CONTRACT_NAME, `${CONTRACT_NAME}.abi`)

describe.only(CONTRACT_NAME, () => {
    let converterRegistryAccount;

    before(async () => {
        converterRegistryAccount = (await newAccount()).accountName;
        await setCode(converterRegistryAccount, CODE_FILE_PATH, ABI_FILE_PATH);
    });
    describe('addconverter', async () => {
        it("verifies adding a valid converter registers all data correctly", async () => {
            await expectNoError(
                addConverter(converterRegistryAccount, bancorConverter, 'BNTEOS')
            )
        })
    });
})
