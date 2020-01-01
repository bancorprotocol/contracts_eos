
const chai = require('chai')
var assert = chai.assert

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
    convertSYS,
    convertBNT,
    convertEOS,
    convertRelay
} = require('./common/token')

const { 
    init,
    update,
    setMaxfee,
    setreserve,
    delreserve,
    getReserve,
    getSettings
} = require('./common/converter')

const { ERRORS } = require('./common/errors')

describe('Test: BancorConverter', () => {
    const user1 = 'bnttestuser1'
    const user2 = 'bnttestuser2'
    const bntStaking = 'stakebnt2eos'
    describe('setup token contracts and issue tokens', function () {
        it('create and issue fakeos token - EOS', async function () {
            await expectNoError(
                setMaxfee('thisisbancor', 30000)
            )
            await expectNoError( 
                create('fakeos', 'fakeos', 'EOS', false) 
            )
            await expectNoError( 
                issue('fakeos', 'fakeos', '10100.0000 EOS', 'setup') 
            )
            const result = await get('fakeos', 'EOS')
            assert.equal(result.rows.length, 1)
            //
            var expected = '250000000.0000 EOS'
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
            var expected = '250000000.0000 SYS'
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
        it('create and issue BNT token', async function () {
            await expectNoError( 
                create('bancorxoneos', 'bntbntbntbnt', 'BNT') 
            )
            await expectNoError( 
                issue('bntbntbntbnt', 'bancorxoneos', '442000.00000000 BNT', 'setup') 
            )
            const result = await get('bntbntbntbnt', 'BNT')
            assert.equal(result.rows.length, 1)
            //
            var expected = '250000000.00000000 BNT'
            assert.equal(result.rows[0].max_supply, expected, "max_supply not set correctly - bnt")
            //
            expected = '442000.00000000 BNT'
            assert.equal(result.rows[0].supply, expected, "supply not set correctly - bnt")     
        })
        it('create and issue relay token - BNTEOS', async function () {
            await expectNoError( 
                create('bnt2eoscnvrt', 'bnt2eosrelay', 'BNTEOS') 
            )
            await expectNoError( 
                issue('bnt2eosrelay', 'bnt2eoscnvrt', '20300.00000000 BNTEOS', 'setup') 
            )    
            const result = await get('bnt2eosrelay', 'BNTEOS')
            assert.equal(result.rows.length, 1)
            //
            var expected = '250000000.00000000 BNTEOS'
            assert.equal(result.rows[0].max_supply, expected, "max_supply not set correctly - bnteos")    
            //
            expected = '20300.00000000 BNTEOS'
            assert.equal(result.rows[0].supply, expected, "supply not set correctly - bnteos")    
        }) 
        it('create and issue relay token - RELAY', async function () {
            await expectNoError( 
                create('bnt2eoscnvrt', 'bnt2eosrelay', 'RELAY') 
            )
            await expectNoError( 
                issue('bnt2eosrelay', 'bnt2eoscnvrt', '20300.00000000 RELAY', 'setup') 
            )    
            const result = await get('bnt2eosrelay', 'RELAY')
            assert.equal(result.rows.length, 1)
            //
            var expected = '250000000.00000000 RELAY'
            assert.equal(result.rows[0].max_supply, expected, "max_supply not set correctly - RELAY")    
            //
            expected = '20300.00000000 RELAY'
            assert.equal(result.rows[0].supply, expected, "supply not set correctly - RELAY")    
        })
        it('create and issue relay token - RELAYB', async function () {
            await expectNoError( 
                create('bnt2eoscnvrt', 'bnt2eosrelay', 'RELAYB') 
            )
            await expectNoError( 
                issue('bnt2eosrelay', 'bnt2eoscnvrt', '20300.00000000 RELAYB', 'setup') 
            )    
            const result = await get('bnt2eosrelay', 'RELAYB')
            assert.equal(result.rows.length, 1)
            //
            var expected = '250000000.00000000 RELAYB'
            assert.equal(result.rows[0].max_supply, expected, "max_supply not set correctly - RELAYB")    
            //
            expected = '20300.00000000 RELAYB'
            assert.equal(result.rows[0].supply, expected, "supply not set correctly - RELAYB")    
        })     
        it('create and issue relay token - BNTSYS', async function () {
            await expectNoError( 
                create('bnt2syscnvrt', 'bnt2sysrelay', 'BNTSYS') 
            )
            await expectNoError( 
                issue('bnt2sysrelay', 'bnt2syscnvrt', '20100.00000000 BNTSYS', 'setup') 
            )
            const result = await get('bnt2sysrelay', 'BNTSYS')
            assert.equal(result.rows.length, 1)
            //
            var expected = '250000000.00000000 BNTSYS'
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
            await expectError( //same key but different account from that of bnt2eos converter owner
                init('bnt2eoscnvrt', user1),
                ERRORS.PERMISSIONS
            )
            await expectNoError( 
                init() // smart_enabled = 0 by default
            )
            result = await getSettings()
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].max_fee, 30000, "max_fee not set correctly - bnteos")
            await expectNoError( 
                init('bnt2syscnvrt', undefined, "bnt2sysrelay", "BNTSYS") // smart_enabled = 0 by default
            )
            result = await getSettings()
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].max_fee, 30000, "max_fee not set correctly - bntsys")
            await expectNoError( 
                init('bnt2aaacnvrt', undefined, "bnt2aaarelay", "BNTAAA", 1400, true)
            )
            await expectNoError( 
                init('bnt2bbbcnvrt', undefined, "bnt2bbbrelay", "BNTBBB", 1400, true)
            )
        })
        it('set reserves eos and bnt for bnteos relay', async function () {
            await expectNoError(
                setreserve()
            )
            result = await getReserve('BNT')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].ratio, 500000, "BNT reserve ratio not set correctly - bnteos")
            //
            await expectNoError(
                setreserve(true, 'aaa','AAA','bnt2aaacnvrt')
            )
            await expectNoError( 
                setreserve(true, 'bntbntbntbnt', 'BNT', 'bnt2aaacnvrt')
            )
            await expectNoError(
                setreserve(true, 'bbb','BBB','bnt2bbbcnvrt')
            )
            await expectNoError( 
                setreserve(true, 'bntbntbntbnt', 'BNT', 'bnt2bbbcnvrt')
            )
            //
            await expectNoError( 
                setreserve(false, 'eosio.token', 'EOS')
            )
            result = await getReserve('EOS')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].ratio, 500000, "EOS reserve ratio not set correctly - bnteos")
        })
        it('set reserves eos and bnt for BNTfakeSYS relay', async function () {
            await expectNoError( 
                setreserve(true, 'bntbntbntbnt', 'BNT', 'bnt2syscnvrt')
            )
            result = await getReserve('BNT')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].ratio, 500000, "BNT reserve ratio not set correctly - bntsys")
            //
            await expectNoError( 
                setreserve(false, 'fakeos', 'SYS', 'bnt2syscnvrt')
            )
            result = await getReserve('SYS')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].ratio, 500000, "SYS reserve ratio not set correctly - bntsys")
        })
        it('transfer eos to user1 and converter', async function () {
            await expectNoError( 
                transfer('eosio.token', '10000.0000 EOS', user1, 'eosio') 
            )
            result = await getBalance(user1, 'eosio.token', 'EOS')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].balance, '10000.0000 EOS', "wrong initial balance - user1's eos")
            
            await expectNoError( 
                transfer('eosio.token', '10000.0000 EOS', user2, 'eosio') 
            )

            await expectNoError( 
                transfer('eosio.token', '10000.0000 EOS', 'bnt2eoscnvrt', 'eosio') 
            )
            result = await getBalance('bnt2eoscnvrt', 'eosio.token', 'EOS')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].balance, '10000.0000 EOS', "wrong initial balance - converter's eos")
        })
        it('transfer fakeos EOS and SYS to user1 and SYS to converter', async function () {
            await expectNoError( 
                transfer('fakeos', '100.0000 EOS', user1, 'fakeos') 
            )
            var result = await getBalance(user1, 'fakeos', 'EOS')
            assert.equal(result.rows.length, 1)    
            assert.equal(result.rows[0].balance, '100.0000 EOS', "wrong initial balance - user1's fakeos EOS")
            //
            await expectNoError( 
                transfer('fakeos', '100.0000 SYS', user1, 'fakeos') 
            )
            var result = await getBalance(user1, 'fakeos', 'SYS')
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
                transfer('bntbntbntbnt','21000.00000000 BNT', user1, 'bancorxoneos') 
            )
            var result = await getBalance(user1, 'bntbntbntbnt', 'BNT')
            assert.equal(result.rows.length, 1)            
            assert.equal(result.rows[0].balance, '21000.00000000 BNT', "wrong initial balance - bnt")
            await expectNoError( 
                transfer('bntbntbntbnt','21000.00000000 BNT', user2, 'bancorxoneos') 
            )
            //
            await expectNoError( 
                transfer('bntbntbntbnt', '10000.00000000 BNT', 'bnt2syscnvrt', 'bancorxoneos') 
            )
            result = await getBalance('bnt2syscnvrt', 'bntbntbntbnt', 'BNT')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].balance, '10000.00000000 BNT', "bnt2sys - bad amount in reserve - bnt")
            //
            await expectNoError(
                transfer('bntbntbntbnt', '10000.00000000 BNT','bnt2eoscnvrt', 'bancorxoneos') 
            )
            result = await getBalance('bnt2eoscnvrt','bntbntbntbnt','BNT')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].balance, '10000.00000000 BNT', "bnt2eos - bad amount in reserve - bnt")
            //
            await expectNoError(
                transfer('bntbntbntbnt', '10000.00000000 BNT','bnt2aaacnvrt', 'bancorxoneos') 
            )
            await expectNoError(
                transfer('bntbntbntbnt', '10000.00000000 BNT','bnt2bbbcnvrt', 'bancorxoneos') 
            )
        })
        it('transfer BNTEOS and BNTSYS to user1, BNTEOS to staking', async function () {
            await expectNoError( //issue BNTEOS to user1
                transfer('bnt2eosrelay',  '100.00000000 BNTEOS', user1, 'bnt2eoscnvrt') 
            )
            var result = await getBalance(user1, 'bnt2eosrelay', 'BNTEOS')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].balance, '100.00000000 BNTEOS', "wrong initial balance - bnteos")
            //
            await expectNoError( //issue BNTEOS to user2
                transfer('bnt2eosrelay',  '100.00000000 BNTEOS', user2, 'bnt2eoscnvrt') 
            )
            await expectNoError( //issue BNTSYS to user2
                transfer(token='bnt2sysrelay', '100.00000000 BNTSYS', user1, 'bnt2syscnvrt') 
            )
            var result = await getBalance(user1, 'bnt2sysrelay', 'BNTSYS')
            assert.equal(result.rows.length, 1)
            assert.equal(result.rows[0].balance, '100.00000000 BNTSYS', "wrong initial balance - bntsys")
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
                init(), ERRORS.SETTINGS_EXIST
            )
        })
        it("trying to buy relays with 'smart_enabled' set to false - should throw (BNT)", async () => { 
            await expectError(
                convertBNT('5.00000000'),
                ERRORS.TOKEN_PURCHASES_DISABLED
            )
        })
        it("trying to sell relays and get less than min_return (BNT)", async () => { 
            await expectError(
                convertRelay('0.00000001'), 
                ERRORS.BELOW_MIN
            )
        })
        it("trying to sell relays with 'smart_enabled' set to false - should not throw (BNT)", async () => { 
            await expectNoError(
                convertRelay('0.00100000')
            )
        })
    })
    describe('test converter updates', function () { 
        it('update fee beyond max_fee', async function () { 
            await expectError(
                update(100000), 
                ERRORS.FEE_OVER_MAX
            )
        })            
        it('update with bad auth', async function () {
            await expectError(
                update(0, 'bnt2eoscnvrt', user1), 
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
        it('update bnt2eos with staking', async function () {
            await expectNoError(
                update()
            ) 
            //convert expect amount
            await expectError(
                convertBNT(0), 
                ERRORS.NO_ZERO
            )
            await expectNoError(
                convertBNT('0.00100000')
            )
        })
    })
    describe('some last invalid ops', function () { 
        it("trying to buy BNTEOS with fakeos - should throw", async () => { 
            await expectError(
                convertEOS('5.0000', true),
                ERRORS.MUST_HAVE_TOKEN_ENTRY
            )
        })
        it("trying to buy BNTEOS with SYS - should throw", async () => { 
            await expectError(
                convertSYS('5.0000'), 
                ERRORS.MUST_HAVE_TOKEN_ENTRY
            )
        })
        it("trying to delete BNT reserve when it's not empty - should throw", async () => { 
            await expectError(
                delreserve('EOS'), 
                "may delete only empty reserves"
            )
        })
        it("trying to delete BNT reserve without permission - should throw", async () => { 
            await expectError(
                delreserve('BNT', user1), 
                "missing authority of bnt2eoscnvrt"
            )
        })
    })
})
