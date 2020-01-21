
const chai = require('chai')
let assert = chai.assert

const config = require('../../config/accountNames.json')
const {  
    expectError, 
    expectNoError,
} = require('./common/utils')

const { 
    get,
    issue,
    create,
    transfer,
    getBalance,
    convertEOS
} = require('./common/token')

const { 
    init,
    update,
    setreserve,
    delreserve,
    getReserve,
    getSettings
} = require('./common/converter')

const { ERRORS } = require('./common/errors')

describe('Test: BancorConverter', () => {
    const user1 = config.MASTER_ACCOUNT
    const user2 = config.TEST_ACCOUNT
    const networkTokenContract = config.BNT_TOKEN_ACCOUNT
    const bancorXContract = config.BANCOR_X_ACCOUNT

    describe('setup token contracts and issue tokens', function () {
        it('create and issue fakeos token - EOS', async function () {
            await expectNoError( 
                create('fakeos', 'fakeos', 'EOS', false) 
            )
            await expectNoError( 
                issue('fakeos', 'fakeos', '10100.0000 EOS', 'setup') 
            )
            const result = await get('fakeos', 'EOS')
            assert.equal(result.rows.length, 1)
            //
            let expected = '250000000.0000 EOS'
            assert.equal(result.rows[0].max_supply, expected, "max_supply not set correctly - fakeos EOS")
            //
            expected = '10100.0000 EOS'
            assert.equal(result.rows[0].supply, expected, "supply not set correctly - fakeos EOS")    
        })
        it('create and issue fakeos token - SYS', async function () {
            await expectNoError( 
                create('fakeos', 'fakeos', 'SYS', false) 
            )
            await expectNoError( 
                issue('fakeos', 'fakeos', '10100.0000 SYS', 'setup') 
            )
            const result = await get('fakeos', 'SYS')
            assert.equal(result.rows.length, 1)
            //
            let expected = '250000000.0000 SYS'
            assert.equal(result.rows[0].max_supply, expected, "max_supply not set correctly - fakeos SYS")    
            //
            expected = '10100.0000 SYS'
            assert.equal(result.rows[0].supply, expected, "supply not set correctly - fakeos SYS")    
        })
        it('create and issue AAA and BBB', async function () {
            await expectNoError( 
                create('aaa', 'aaa', 'AAA') 
            )
            await expectNoError( 
                create('bbb', 'bbb', 'BBB') 
            )
            await expectNoError( 
                issue('aaa', 'aaa', '10200.00000000 AAA', 'setup') 
            )
            await expectNoError( 
                issue('bbb', 'bbb', '10200.00000000 BBB', 'setup') 
            )
        })
        it('transfer AAA and BBB tokens to users', async function () {   
            await expectNoError(
                transfer('aaa', '100.00000000 AAA', user1) 
            )
            await expectNoError(
                transfer('aaa', '100.00000000 AAA', user2) 
            )
            await expectNoError(
                transfer('bbb', '100.00000000 BBB', user1) 
            )
            await expectNoError(
                transfer('bbb', '100.00000000 BBB', user2) 
            )  
        })
        it('create and issue relay token - BNTSYS', async function () {
            await expectNoError( 
                create('bnt2syscnvrt', 'bnt2sysrelay', 'BNTSYS') 
            )
            await expectNoError( 
                issue('bnt2sysrelay', 'bnt2syscnvrt', '10100.00000000 BNTSYS', 'setup') 
            )
            await expectNoError( 
                issue('bnt2sysrelay', 'bnt2syscnvrt', '10000.00000000 BNTSYS', '') 
            )
            await expectNoError(
                transfer('bnt2sysrelay', '10000.00000000 BNTSYS', user1, 'bnt2syscnvrt') 
            )  
            const result = await get('bnt2sysrelay', 'BNTSYS')
            assert.equal(result.rows.length, 1)
            //
            let expected = '250000000.00000000 BNTSYS'
            assert.equal(result.rows[0].max_supply, expected, "max_supply not set correctly - bntsys")
            //
            expected = '20100.00000000 BNTSYS'
            assert.equal(result.rows[0].supply, expected, "max_supply not set correctly - bntsys")    
        })
        it('create and issue BNTAAA and BNTBBB tokens', async function () {
            await expectNoError( 
                create('bnt2aaacnvrt', 'bnt2aaarelay', 'BNTAAA') 
            )
            await expectNoError( 
                issue('bnt2aaarelay', 'bnt2aaacnvrt', '10200.00000000 BNTAAA', 'setup') 
            )
            await expectNoError( 
                create('bnt2bbbcnvrt', 'bnt2bbbrelay', 'BNTBBB') 
            )
            await expectNoError( 
                issue('bnt2bbbrelay', 'bnt2bbbcnvrt', '10200.00000000 BNTBBB', 'setup') 
            )
        })
        it('transfer BNTAAA and BNTBBB tokens', async function () {
            await expectNoError(
                transfer('bnt2aaarelay', '100.00000000 BNTAAA', user1, 'bnt2aaacnvrt') 
            )
            await expectNoError(
                transfer('bnt2aaarelay', '100.00000000 BNTAAA', user2, 'bnt2aaacnvrt') 
            )
            await expectNoError(
                transfer('bnt2bbbrelay', '100.00000000 BNTBBB', user1, 'bnt2bbbcnvrt') 
            )
            await expectNoError(
                transfer('bnt2bbbrelay', '100.00000000 BNTBBB', user2, 'bnt2bbbcnvrt') 
            )
        })
    })
    describe('setup converters and transfer tokens', function () {
        it('initialize the converters', async function () {
            await expectNoError( 
                init('bnt2syscnvrt', undefined, "bnt2sysrelay", "BNTSYS") // smart_enabled = 0 by default
            )
            result = await getSettings('bnt2syscnvrt')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].max_fee, 30000, "max_fee not set correctly - bntsys")
            await expectNoError( 
                init('bnt2aaacnvrt', undefined, "bnt2aaarelay", "BNTAAA", 1400, true)
            )
            await expectNoError( 
                init('bnt2bbbcnvrt', undefined, "bnt2bbbrelay", "BNTBBB", 0, true)
            )
        })
        it('set reserves eos and bnt for bnteos relay', async function () {
            await expectNoError(
                setreserve(true, 'aaa','AAA','bnt2aaacnvrt')
            )
            await expectNoError( 
                setreserve(true, networkTokenContract, 'BNT', 'bnt2aaacnvrt')
            )
            await expectNoError(
                setreserve(true, 'bbb','BBB','bnt2bbbcnvrt')
            )
            await expectNoError( 
                setreserve(true, networkTokenContract, 'BNT', 'bnt2bbbcnvrt')
            )
        })
        it('set reserves eos and bnt for BNTfakeSYS relay', async function () {
            await expectNoError( 
                setreserve(true, networkTokenContract, 'BNT', 'bnt2syscnvrt')
            )
            result = await getReserve('BNT', 'bnt2syscnvrt')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].ratio, 500000, "BNT reserve ratio not set correctly - bntsys")
            //
            await expectNoError( 
                setreserve(false, 'fakeos', 'SYS', 'bnt2syscnvrt')
            )
            result = await getReserve('SYS', 'bnt2syscnvrt')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].ratio, 500000, "SYS reserve ratio not set correctly - bntsys")
        })
        it('transfer fakeos EOS and SYS to user1 and SYS to converter', async function () {
            await expectNoError( 
                transfer('fakeos', '100.0000 EOS', user1, 'fakeos') 
            )
            let result = await getBalance(user1, 'fakeos', 'EOS')
            assert.equal(result.rows.length, 1)    
            assert.equal(result.rows[0].balance, '100.0000 EOS', "wrong initial balance - user1's fakeos EOS")
            //
            await expectNoError( 
                transfer('fakeos', '100.0000 SYS', user1, 'fakeos') 
            )
            result = await getBalance(user1, 'fakeos', 'SYS')
            assert.equal(result.rows.length, 1)    
            assert.equal(result.rows[0].balance, '100.0000 SYS', "wrong initial balance - user1's fakeos SYS")
            //
            await expectNoError( 
                transfer('fakeos', '10000.0000 SYS', 'bnt2syscnvrt') 
            )
            result = await getBalance('bnt2syscnvrt','fakeos','SYS')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].balance, '10000.0000 SYS', "bad amount in reserve - fakeos SYS")
        })
        it('transfer BNT to user1, user2 and converters', async function () {
            await expectNoError( 
                transfer(networkTokenContract,'21000.00000000 BNT', user1, bancorXContract) 
            )
            await expectNoError( 
                transfer(networkTokenContract,'21000.00000000 BNT', user2, bancorXContract) 
            )
            //
            await expectNoError( 
                transfer(networkTokenContract, '10000.00000000 BNT', 'bnt2syscnvrt', bancorXContract) 
            )
            await expectNoError(
                transfer(networkTokenContract, '10000.00000000 BNT','bnt2aaacnvrt', bancorXContract) 
            )
            await expectNoError(
                transfer(networkTokenContract, '10000.00000000 BNT','bnt2bbbcnvrt', bancorXContract) 
            )
        })
        it('transfer AAA and BBB to converters', async function () {
            await expectNoError(
                transfer('aaa', '10000.00000000 AAA', 'bnt2aaacnvrt') 
            )
            await expectNoError(
                transfer('bbb', '10000.00000000 BBB', 'bnt2bbbcnvrt') 
            )
        })
    })
    describe('do stuff with converters', function () {
        it("cannot call init more than once", async() => {
            await expectError(
                init('bnt2syscnvrt'), ERRORS.SETTINGS_EXIST
            )
        })
    })
    describe('test converter updates', function () { 
        it('update fee beyond max_fee', async function () { 
            await expectError(
                update(100000, 'bnt2syscnvrt'), 
                ERRORS.FEE_OVER_MAX
            )
        })            
        it('update with bad auth', async function () {
            await expectError(
                update(0, 'bnt2syscnvrt', user1), 
                ERRORS.PERMISSIONS
            )
        }) 
        it('update fee for SYS', async function () {
            await expectNoError(
                update(10000, "bnt2syscnvrt")
            ) 
            result = await getSettings("bnt2syscnvrt")
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].fee, 10000, "fee not set correctly")
        })
    })
    describe('some last invalid ops', function () { 
        it("trying to buy BNTEOS with fakeos - should throw", async () => { 
            await expectError(
                convertEOS('5.0000', true),
                ERRORS.MUST_HAVE_TOKEN_ENTRY
            )
        })
        it("trying to delete BNT reserve when it's not empty - should throw", async () => { 
            await expectError(
                delreserve('SYS', 'bnt2syscnvrt', 'bnt2syscnvrt'), 
                "may delete only empty reserves"
            )
        })
        it("trying to delete BNT reserve without permission - should throw", async () => { 
            await expectError(
                delreserve('BNT', user1, 'bnt2syscnvrt'), 
                "missing authority of bnt2syscnvrt"
            )
        })
    })
})
