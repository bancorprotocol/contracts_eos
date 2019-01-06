const assert = require('chai').should();
const {
    ensureContractAssertionError,
    getEos,
    snooze
} = require('./utils');
const { ERRORS } = require('./constants');


describe('BancorX Contract', () => {
    const bancorXContract = 'bancorx';
    const networkTokenSymbol = "BNT";
    const networkToken = 'bnt';
    const testUser = 'test1';
    const reporter1User = 'reporter1';
    const reporter2User = 'reporter2';
    const transferId = Math.floor(100000 + Math.random() * 900000)   

    it('should throw when attempting to transfer more BNT than the user holds', async function() {
        let pass = false
        const token = await getEos(testUser).contract(networkToken);
        try {
            let res = await token.transfer({
                from: testUser,
                to: bancorXContract,
                quantity: `999999.0000000000 ${networkTokenSymbol}`,
                memo: `1.1,eth,ETH_ADDRESS`
            }, {
                authorization: [`${testUser}@active`]
            });
        } catch (err) {
            pass = true
        }
        pass.should.be.equal(true)

    });

    it('should throw when calling xTransfer with wrong permissions', async function() {
        let pass = false
        const token = await getEos(testUser).contract(networkToken);
        try {
            let res = await token.transfer({
                from: 'reporter1',
                to: bancorXContract,
                quantity: `2.0000000000 ${networkTokenSymbol}`,
                memo: `1.1,eth,ETH_ADDRESS`
            }, {
                authorization: [`${testUser}@active`]
            });
        } catch (err) {
            pass = true
        }
        pass.should.be.equal(true)

    });

    it('should throw when attempting to call xTransfer when disabled', async () => {
        let pass = false
        const reporter = await getEos(bancorXContract).contract(bancorXContract);
        await reporter.enablext({
            enable: 0
        }, {
            authorization: [`${bancorXContract}@active`]
        })
        try {
            const token = await getEos(testUser).contract(networkToken);
            let res = await token.transfer({
                from: testUser,
                to: bancorXContract,
                quantity: `2.0000000000 ${networkTokenSymbol}`,
                memo: `1.1,eth,ETH_ADDRESS`
            }, {
                authorization: [`${testUser}@active`]
            });
        } catch (err) {
            pass = true
        }
        pass.should.be.equal(true)
        await reporter.enablext({
            enable: 1
        }, {
            authorization: [`${bancorXContract}@active`]
        })
    })
    it('should destroy BNT and emit the right events when calling xTransfer successfully', async function() {
        const token = await getEos(testUser).contract(networkToken);
        let res = await token.transfer({
            from: testUser,
            to: bancorXContract,
            quantity: `2.0000000000 ${networkTokenSymbol}`,
            memo: `1.1,eth,ETH_ADDRESS`
        }, {
            authorization: [`${testUser}@active`]
        });
        var events = res.processed.action_traces[0].inline_traces[1].console;
        events = events.split('\n');

        var destroyEvent = JSON.parse(events[0]);
        var xtransferEvent = JSON.parse(events[1]);
        assert.equal(destroyEvent.etype, "destroy", "unexpected destroy etype result");
        assert.equal(destroyEvent.from, "test1", "unexpected destroy from result");
        assert.equal(destroyEvent.quantity, `2.0000000000 ${networkTokenSymbol}`, "unexpected destroy quantity result");
        assert.equal(xtransferEvent.etype, "xtransfer", "unexpected xtransfer etype result");
        assert.equal(xtransferEvent.blockchain, "eth", "unexpected xtransfer blockchain result");
        assert.equal(xtransferEvent.target, 'ETH_ADDRESS', "unexpected xtransfer target result");
        assert.equal(xtransferEvent.quantity, `2.0000000000 ${networkTokenSymbol}`, "unexpected xtransfer quantity result");

    });


    it('should properly update the tables after 1/2 successful reports', async () => {

        const banx = await getEos(reporter1User).contract(bancorXContract);
        await banx.reporttx({
            tx_id: `${transferId}`,
            reporter: reporter1User,
            target: testUser,
            quantity: `2.0000000000 ${networkTokenSymbol}`,
            memo: 'text',
            data: 'txHash',
            blockchain: 'eth',
            x_transfer_id: '0'
        }, {
            authorization: [`${reporter1User}@active`]
        });


        const transfers = await getEos(bancorXContract).getTableRows({
            code: bancorXContract,
            scope: bancorXContract,
            table: 'transfers',
            json: true,
            key: transferId
        });

        transfers.rows[0].tx_id.should.be.equal(transferId);
        transfers.rows[0].target.should.be.equal('test1');
        transfers.rows[0].quantity.should.be.equal('2.0000000000 BNT');
        transfers.rows[0].blockchain.should.be.equal('eth');
        transfers.rows[0].memo.should.be.equal('text');
        transfers.rows[0].data.should.be.equal('txHash');
        transfers.rows[0]['x_transfer_id'].should.be.equal(0);
        transfers.rows[0].reporters.should.include('reporter1');
        let balance = await getEos(networkToken).getTableRows({
            code: networkToken,
            scope: testUser,
            table: 'accounts',
            json: true,
        });
        balance["rows"][0]["balance"].should.be.equal('9998.0000000000 BNT')

    });

    it('should properly update the tables and issue BNT after 2/2 successful reports', async () => {
        const reporter = await getEos(bancorXContract).contract(bancorXContract);
        await reporter.enablerpt({
            enable: 1
        }, {
            authorization: [`${bancorXContract}@active`]
        })
        const banx = await getEos(reporter2User).contract(bancorXContract);

        let res = await banx.reporttx({
            tx_id: `${transferId}`,
            reporter: reporter2User,
            target: testUser,
            quantity: `2.0000000000 ${networkTokenSymbol}`,
            memo: 'text',
            data: 'txHash',
            blockchain: 'eth',
            x_transfer_id: '0'
        }, {
            authorization: [`${reporter2User}@active`]
        });


        let balance = await getEos(networkToken).getTableRows({
            code: networkToken,
            scope: testUser,
            table: 'accounts',
            json: true,
        });
        balance["rows"][0]["balance"].should.be.equal('10000.0000000000 BNT')

    })

    it('should throw when calling xTransfer with a token other than BNT', async function() {
        const token = await getEos(testUser).contract(networkToken);
        const p = token.transfer({
            from: testUser,
            to: bancorXContract,
            quantity: `2.0000000000 FAKE`,
            memo: `1.1,eth,ETH_ADDRESS`
        }, {
            authorization: [`${testUser}@active`]
        });
        await ensureContractAssertionError(p, ERRORS.NO_KEY);
    });

    it('should throw when a non-approved reporter attempts to report a transaction', async () => {

        const banx = await getEos(testUser).contract(bancorXContract);
        const p = banx.reporttx({
            tx_id: `${transferId}`,
            reporter: reporter1User,
            target: testUser,
            quantity: `2.0000000000 ${networkTokenSymbol}`,
            memo: 'text',
            data: 'txHash',
            blockchain: 'eth',
            x_transfer_id: '0'
        }, {
            authorization: [`${testUser}@active`]
        });

        await ensureContractAssertionError(p, ERRORS.PERMISSIONS);

    });


    it('should throw when reporters give conflicting data', async () => {
        let bancorX = await getEos(reporter1User).contract(bancorXContract);
        await bancorX.reporttx({
            tx_id: `${transferId}`,
            reporter: reporter1User,
            target: testUser,
            quantity: `2.0000000000 ${networkTokenSymbol}`,
            memo: 'text',
            data: 'txHash',
            blockchain: 'eth',
            x_transfer_id: '0'
        }, {
            authorization: [`${reporter1User}@active`]
        });

        bancorX = await getEos(reporter2User).contract(bancorXContract);
        const p = bancorX.reporttx({
            tx_id: `${transferId}`,
            reporter: reporter2User,
            target: testUser,
            quantity: `2.0000000001 ${networkTokenSymbol}`,
            memo: 'text',
            data: 'txHash',
            blockchain: 'eth',
            x_transfer_id: '0'
        }, {
            authorization: [`${reporter2User}@active`]
        });
        await ensureContractAssertionError(p, ERRORS.TRANSFER_DATA_MISMATCH);
    })

    it('should throw when trying to add an already existing reporter', async () => {
        const bancorX = await getEos(bancorXContract).contract(bancorXContract);
        await bancorX.addreporter({
            reporter: 'reporter4'
        }, {
            authorization: `${bancorXContract}@active`
        });
        await snooze(1000);
        const p = bancorX.addreporter({
            reporter: 'reporter4'
        }, {
            authorization: `${bancorXContract}@active`
        });
        await ensureContractAssertionError(p, ERRORS.REPORTER_ALREADY_DEFINED);
    })
    
    it('should throw when trying to remove a non-approved reporter', async () => {
        const bancorX = await getEos(bancorXContract).contract(bancorXContract);
        await bancorX.rmreporter({
            reporter: 'reporter3'
        }, {
            authorization: `${bancorXContract}@active`
        });
        await snooze(1000);
        const p = bancorX.rmreporter({
            reporter: 'reporter3'
        }, {
            authorization: `${bancorXContract}@active`
        });
        await ensureContractAssertionError(p, ERRORS.REPORTER_DOESNT_EXIST);
    })

    it('should throw when somoneone other than the owner attempts to update the contracts private state variables', async () => {
        const bancorX = await getEos(testUser).contract(bancorXContract);
        const p = bancorX.update({
            min_reporters: 2,
            min_limit: 1,
            limit_inc: 100000000000000,
            max_issue_limit: 10000000000000000,
            max_destroy_limit: 10000000000000000},
            {authorization: `${testUser}@active`});
        await ensureContractAssertionError(p, ERRORS.PERMISSIONS);
    })

    it('verifies that the owner can update the contracts private state variables', async () => {
        const bancorX = await getEos(bancorXContract).contract(bancorXContract);
        await bancorX.update({
            min_reporters: 1,
            min_limit: 1,
            limit_inc: 100000009990000,
            max_issue_limit: 100000555000000,
            max_destroy_limit: 10000000666000000},
            {authorization: `${bancorXContract}@active`});
        const settings = await getEos(bancorXContract).getTableRows({
            code: bancorXContract,
            scope: bancorXContract,
            table: 'settings',
            json: true
        });
        settings.rows[0].min_reporters.should.be.equal(1);
        settings.rows[0].min_limit.should.be.equal(1);
        settings.rows[0].limit_inc.should.be.equal('100000009990000');
        settings.rows[0].max_issue_limit.should.be.equal('100000555000000');
        settings.rows[0].max_destroy_limit.should.be.equal('10000000666000000');
    })

});