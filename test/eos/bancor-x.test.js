const assert = require('chai').should();
const {
    expectError,
    expectNoError,
    snooze,
    getTableRows,
    randomAmount
} = require('./common/utils');
const {
    getBalance,
    transfer
} = require('./common/token')

const {
    enablext,
    reporttx,
    update,
    enablerpt,
    addreporter,
    rmreporter
} = require('./common/bancor-x');
const { ERRORS } = require('./common/errors');


describe('BancorX', () => {
    const bancorXContract = 'bancorxoneos';
    const networkTokenSymbol = "BNT";
    const networkToken = 'bntbntbntbnt';
    const testUser = 'bnttestuser1';
    const reporter1User = 'reporter1';
    const reporter2User = 'reporter2';

    it('should throw when attempting to call xTransfer when disabled', async () => {
        await expectNoError(
            enablext(false)
        );
        await expectError(
            transfer(networkToken, `2.00000000 ${networkTokenSymbol}`, bancorXContract, testUser),
            ERRORS.X_TRANSFERS_DISABLED
        );

        await expectNoError(
            enablext(true)
        );
    })

    it('should throw when attempting to call reporttx when reporting is disabled', async () => {
        const transferId = Math.floor(100000 + Math.random() * 900000)

        await expectNoError(
            enablerpt(false)
        );
        await expectError(
            reporttx({ tx_id: transferId, reporter: reporter1User }),
            ERRORS.REPORTING_DISABLED
        );

        await expectNoError(
            enablerpt(true)
        );
    })
    

    it('should destroy BNT and emit the right events when calling xTransfer successfully', async function() {
        const res = await transfer(networkToken, `2.00000000 ${networkTokenSymbol}`, bancorXContract, testUser, '1.1,eth,ETH_ADDRESS');
        
        const events = res.processed.action_traces[0].inline_traces[1].console.split('\n');
        
        const destroyEvent = JSON.parse(events[0]);
        const xtransferEvent = JSON.parse(events[1]);

        assert.equal(destroyEvent.etype, "destroy", "unexpected destroy etype result");
        assert.equal(destroyEvent.from, testUser, "unexpected destroy from result");
        assert.equal(destroyEvent.quantity, `2.00000000 ${networkTokenSymbol}`, "unexpected destroy quantity result");
        assert.equal(xtransferEvent.etype, "xtransfer", "unexpected xtransfer etype result");
        assert.equal(xtransferEvent.blockchain, "eth", "unexpected xtransfer blockchain result");
        assert.equal(xtransferEvent.target, 'ETH_ADDRESS', "unexpected xtransfer target result");
        assert.equal(xtransferEvent.quantity, `2.00000000 ${networkTokenSymbol}`, "unexpected xtransfer quantity result");

    });


    it('ensures reporttx creates a valid transfer document, and releases funds once all reporters report', async () => {
        const transferId = Math.floor(100000 + Math.random() * 900000)
        const amount = randomAmount({});
        const quantity = `${amount} ${networkTokenSymbol}`;

        const initialBalance = Number((await getBalance(testUser, networkToken, networkTokenSymbol)).rows[0].balance.split(' ')[0])

        await expectNoError(
            reporttx({ tx_id: transferId, reporter: reporter1User, quantity })
        );

        const transfer = (await getTableRows(bancorXContract, bancorXContract, 'transfers')).rows
            .find(({ tx_id }) => tx_id === transferId);

        transfer.tx_id.should.be.equal(transferId);
        transfer.target.should.be.equal(testUser);
        transfer.quantity.should.be.equal(quantity);
        transfer.blockchain.should.be.equal('eth');
        transfer.memo.should.be.equal('enjoy your BNTs');
        transfer.data.should.be.equal('txHash');
        transfer.reporters.should.include(reporter1User);

        await expectNoError(
            reporttx({ tx_id: transferId, reporter: reporter2User, quantity })
        );

        const fullfiledTransfer = (await getTableRows(bancorXContract, bancorXContract, 'transfers')).rows
            .find(({ tx_id }) => tx_id === transferId);
        
        assert.equal(fullfiledTransfer, undefined);
        
        const finalBalance = Number((await getBalance(testUser, networkToken, networkTokenSymbol)).rows[0].balance.split(' ')[0])
        
        finalBalance.toFixed(8).should.be.equal((initialBalance + Number(amount)).toFixed(8));
    });

    it('ensures no event is emitted when to xTransfer using an unknown token', async function() {
        const quantity = `${randomAmount({ decimals: 4 })} EOS`;
        const res = await expectNoError(
            transfer('eosio.token', quantity, bancorXContract, testUser, `1.1,eth,ETH_ADDRESS`)
        );
        
        JSON.stringify(res.processed.action_traces).should.not.include('xtransfer');
        JSON.stringify(res.processed.action_traces).should.not.include('destory');
    });

    it('should throw when a non-approved reporter attempts to report a transaction', async () => {
        const transferId = Math.floor(100000 + Math.random() * 900000)

        await expectError(
            reporttx({ tx_id: transferId, reporter: testUser }),
            ERRORS.UNKNOWN_REPORTER
        );
    });


    it('should throw when reporters give conflicting data', async () => {
        const transferId = Math.floor(100000 + Math.random() * 900000)

        await expectNoError(
            reporttx({ tx_id: transferId, reporter: reporter1User })
        );
        
        await expectError(
            reporttx({ tx_id: transferId, reporter: reporter2User, quantity: `2.00000001 ${networkTokenSymbol}` }),
            ERRORS.TRANSFER_DATA_MISMATCH
        );
    })

    it('should throw when the same reporter reports a transaction twice', async () => {
        const transferId = Math.floor(100000 + Math.random() * 900000)
        const quantity = `${randomAmount({})} ${networkTokenSymbol}`;

        await expectNoError(
            reporttx({ tx_id: transferId, reporter: reporter1User, quantity })
        );

        await snooze(500) // avoid getting a duplicate transaction error from nodeos

        await expectError(
            reporttx({ tx_id: transferId, reporter: reporter1User, quantity }),
            ERRORS.DUPLICATE_REPORT
        );
    })

    it('should throw when trying to add an already existing reporter', async () => {
        const reporter = 'reporter4';

        try {
            await expectNoError(
                addreporter(reporter)
            );
    
            await snooze(500) // avoid getting a duplicate transaction error from nodeos
    
            await expectError(
                addreporter(reporter),
                ERRORS.REPORTER_ALREADY_DEFINED
            );
        }
        finally {
            // Revert
            await expectNoError(
                rmreporter(reporter)
            );
        }
    })
    
    it('should throw when trying to remove a non-approved reporter', async () => {
        await expectError(rmreporter(testUser), ERRORS.REPORTER_DOESNT_EXIST);
    })

    it('should throw when somoneone other than the owner attempts to update the contracts private state variables', async () => {
        const p = update({
            min_reporters: 2,
            min_limit: 1,
            limit_inc: 100000000000000,
            max_issue_limit: 10000000000000000,
            max_destroy_limit: 10000000000000000
            },
            { actor: testUser, permission: 'active' });
        await expectError(p, ERRORS.PERMISSIONS);
    })

});