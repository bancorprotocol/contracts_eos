const { ensureContractAssertionError, getEos, snooze } = require('./utils');
const { ERRORS } = require('./constants');
const assert = require('assert')

describe('XTransferRerouter', () => {
    const rerouteContract = 'txrerouter';
    const testUser = 'test1';

    it('verify that a user can\'t call reroutetx before rrt_enabled is not set', async function() {
        const rerouter = await getEos(testUser).contract(rerouteContract);
        const p = rerouter.reroutetx({tx_id: 555,
            blockchain: 'eth',
            target: '0x666'
            },{authorization: `${testUser}@active`});
        await ensureContractAssertionError(p, ERRORS.SINGLETON_DOESNT_EXIST);
    });

    it('verify that a non-owner can\'t call enablerrt', async function() {
        const rerouter = await getEos(testUser).contract(rerouteContract);
        const p = rerouter.enablerrt({
            enable: 1
            },{authorization: `${testUser}@active`});
        await ensureContractAssertionError(p, ERRORS.PERMISSIONS);
    });

    it('verify that an owner can call enablerrt', async function() {
        const rerouter = await getEos(rerouteContract).contract(rerouteContract);
        await rerouter.enablerrt({
            enable: 1
            },{authorization: `${rerouteContract}@active`});
    });

    it('verify that a user can call reroutetx when rerouting is enabled, and that an event is emitted', async function() {
        const rerouter = await getEos(testUser).contract(rerouteContract);
        const stdout = await rerouter.reroutetx({
            tx_id: 666,
            blockchain: 'eth',
            target: '0x666'
            },{authorization: `${testUser}@active`});
        const event = stdout.processed.action_traces[0].console.split('\n')[0]
        const expected_event = '{"version":"1.1","etype":"txreroute","tx_id":"666","blockchain":"eth","target":"0x666"}'
        assert(event === expected_event, 'txroute event was not emitted')
    });

    it('verify that a user can\'t call reroutetx when rerouting is disabled', async function() {
        let rerouter = await getEos(rerouteContract).contract(rerouteContract);
        await rerouter.enablerrt({
            enable: 0
            },{authorization: `${rerouteContract}@active`});
        rerouter = await getEos(testUser).contract(rerouteContract);
        const p = rerouter.reroutetx({tx_id: 667,
            blockchain: 'eth',
            target: '0x666'
            },{authorization: `${testUser}@active`});
        await ensureContractAssertionError(p, ERRORS.REROUTING_DISABLED); 
    });
});