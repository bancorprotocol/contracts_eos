require("babel-core/register");
require("babel-polyfill");
const assert = require('chai').should();
const { ERRORS } = require('./constants');

const {
    ensureContractAssertionError,
    getEos,
    snooze
} = require('./utils');

const networkContract = 'thisisbancor';
const tokenAContract= 'aa';
const tokenASymbol = "TKNA";
const converter1 = 'cnvtaa';
const bntConverter = 'bnt2eoscnvrt';
const bancorXContract = 'bancorxoneos';
const networkTokenSymbol = "BNT";
const networkToken = 'bntbntbntbnt';
const testUser = 'test1';
const reporter1User = 'reporter1';
const reporter2User = 'reporter2';

describe('Cross Conversions', () => {

    it('can transfer BNT by id after being issued by bancorx', async function() {
        const tx_id = getRandomId()
        const x_transfer_id = getRandomId()

        const prevBalance = await getBalance(testUser, networkToken)

        await reportAndIssue(tx_id, testUser, `10.0000000000 BNT`, 'hi', 'data', 'eth', x_transfer_id)

        const currBalance = await getBalance(testUser, networkToken)

        assert.equal(currBalance - prevBalance, 10, "unexpected amount of BNT issued")

        const token = await getEos(testUser).contract(networkToken);

        let res = await token.transferbyid({
            from: testUser,
            to: reporter1User,
            amount_account: bancorXContract,
            amount_id: x_transfer_id,
            memo: "hi"
        }, {
            authorization: [`${testUser}@active`]
        })

        const postBalance = await getBalance(testUser, networkToken)

        assert.equal(postBalance, prevBalance, "unexpected balance after transferring BNT by id")
    })

    it("can convert BNT to another token by amount_id rather than quantity", async function() {
        const tx_id = getRandomId()
        const x_transfer_id = getRandomId()

        await reportAndIssue(tx_id, testUser, `10.0000000000 BNT`, 'hi', 'data', 'eth', x_transfer_id)

        const token = await getEos(testUser).contract(networkToken);

        const prevBalance = await getBalance(testUser, tokenAContract)

        let res = await token.transferbyid({
            from: testUser,
            to: networkContract,
            amount_account: bancorXContract,
            amount_id: x_transfer_id,
            memo: `1,${converter1} ${tokenASymbol},1.00000000,${testUser}`
        }, {
            authorization: [`${testUser}@active`]
        })

        const postBalance = await getBalance(testUser, tokenAContract)

        assert.equal(postBalance > prevBalance, true, "post balance not greater than than previous balance")
    })

    it("can convert and xtransfer in one action", async function() {
        const x_transfer_id = getRandomId()

        const token = await getEos(testUser).contract(tokenAContract);

        let res = await token.transfer({
            from: testUser,
            to: networkContract,
            quantity: `5.00000000 ${tokenASymbol}`,
            memo: `1,${converter1} ${networkTokenSymbol},1.0000000000,${bancorXContract};1.1,eth,0x12345123451234512345,${x_transfer_id}`
        }, {
            authorization: [`${testUser}@active`]
        })

        let bancorXEvents = JSON.parse(res.processed.action_traces[0].inline_traces[2].inline_traces[2].inline_traces[1].console.split("\n")[1])
        // console.log(bancorXEvents)
        assert.equal(bancorXEvents.etype, "xtransfer")
        assert.equal(Number(bancorXEvents.id), x_transfer_id)
        assert.equal(bancorXEvents.blockchain, "eth")
        assert.equal(bancorXEvents.target, "0x12345123451234512345")
    })

    it("doesn't allow a user to call transferbyid without the right authorities", async function() {
        const tx_id = getRandomId()
        const x_transfer_id = getRandomId()

        await reportAndIssue(tx_id, testUser, `10.0000000000 BNT`, 'hi', 'data', 'eth', x_transfer_id)

        const token = await getEos(reporter2User).contract(networkToken);

        const p = token.transferbyid({
            from: testUser,
            to: networkContract,
            amount_account: bancorXContract,
            amount_id: x_transfer_id,
            memo: `1,${converter1} ${tokenASymbol},1.00000000,${testUser}`
        }, {
            authorization: [`${reporter2User}@active`]
        })

        await ensureContractAssertionError(p, ERRORS.PERMISSIONS)
    })
})

const reportAndIssue = async (tx_id, target, quantity, memo, data, blockchain, x_transfer_id) => {
    for (let i = 1; i <= 2; i++) {
        let banx = await getEos(`reporter${i}`).contract(bancorXContract)
        await banx.reporttx({
            tx_id: tx_id,
            reporter: `reporter${i}`,
            target: target,
            quantity: quantity,
            memo: memo,
            data: data,
            blockchain: blockchain,
            x_transfer_id: x_transfer_id
        }, {
            authorization: [`reporter${i}@active`]
        });
    }
}

const getBalance = async (account, tokenContract) => {
    let balance = await getEos(tokenContract).getTableRows({
        code: tokenContract,
        scope: account,
        table: 'accounts',
        json: true,
    });
    return balance["rows"][0]["balance"].split(" ")[0]
}

const getRandomId = () => Math.floor(100000 + Math.random() * 900000)