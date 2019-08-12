require("babel-core/register");
require("babel-polyfill");
const { ERRORS } = require('./constants');

const {
    ensureContractAssertionError,
    getEos,
    ensurePromiseDoesntThrow
} = require('./utils');

const networkContract = 'thisisbancor';
const tokenCContract= 'cc';
const tokenCSymbol = "TKNC";
const tokenCRelaySymbol = "BNTTKNC";
const converterC = 'cnvtcc';
const bntConverter = 'bnt2eoscnvrt';
const networkTokenSymbol = "BNT";
const bntRelaySymbol = 'BNTEOS';
const bntRelay = 'bnt2eosrelay';
const networkToken = 'bntbntbntbnt';
const testUser = 'test1';

describe('BancorConverter', () => {
    it("trying to buy relays with 'smart_enabled' set to false - should throw (BNT)", async () => {
        const bntToken = await getEos(testUser).contract(networkToken);
        const minReturn = '0.0000000001';

        const conversion = bntToken.transfer(
            {
                from: testUser,
                to: networkContract,
                quantity: `5.0000000000 ${networkTokenSymbol}`,
                memo: `1,${bntConverter} ${bntRelaySymbol},${minReturn},${testUser}`
            },
            { authorization: `${testUser}@active` }
        );

        await ensureContractAssertionError(conversion, ERRORS.TOKEN_PURCHASES_DISABLED);
    });

    it("trying to sell relays with 'smart_enabled' set to false - should not throw (BNT)", async () => {
        const bntRelayTokenIssuer = await getEos(bntConverter).contract(bntRelay);
        const bntRelayTokenTestUser = await getEos(testUser).contract(bntRelay);

        const minReturn = '0.0000000001';

        await bntRelayTokenIssuer.issue({
            to: testUser,
            quantity: `10.0000000000 ${bntRelaySymbol}`,
            memo: "hey there"
        }, { authorization: `${bntConverter}@active` });

        const p = bntRelayTokenTestUser.transfer(
            {
                from: testUser,
                to: networkContract,
                quantity: `10.0000000000 ${bntRelaySymbol}`,
                memo: `1,${bntConverter} ${networkTokenSymbol},${minReturn},${testUser}`
            },
            { authorization: `${testUser}@active` }
        );   

        await ensurePromiseDoesntThrow(p);
    });

    it("trying to buy relays with 'smart_enabled' set to true - should not throw", async () => {
        const tokenAIssuer = await getEos(tokenCContract).contract(tokenCContract);
        const tokenATestUser = await getEos(testUser).contract(tokenCContract);

        const minReturn = '0.0000000001';

        await tokenAIssuer.issue({
            to: testUser,
            quantity: `10.00000000 ${tokenCSymbol}`,
            memo: "hey there"
        }, { authorization: `${tokenCContract}@active` });

        const p = tokenATestUser.transfer(
            {
                from: testUser,
                to: networkContract,
                quantity: `10.00000000 ${tokenCSymbol}`,
                memo: `1,${converterC} ${tokenCRelaySymbol},${minReturn},${testUser}`
            },
            { authorization: `${testUser}@active` }
        );

        await ensurePromiseDoesntThrow(p);
    });
});