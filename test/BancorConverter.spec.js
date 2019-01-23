require("babel-core/register");
require("babel-polyfill");
const assert = require('chai').should();
const { ERRORS } = require('./constants');

const {
    ensureContractAssertionError,
    getEos,
    snooze
} = require('./utils');

const networkContract = 'bancornetwrk';
const tokenAContract= 'aa';
const tokenASymbol = "TKNA";
const converter1 = 'cnvtaa';
const bntConverter = 'bnt2eoscnvrt';
const bancorXContract = 'bancorx';
const networkTokenSymbol = "BNT";
const bntRelaySymbol = 'BNTEOS';
const networkToken = 'bnt';
const testUser = 'test1';

describe('BancorConverter', () => {
    it.only("trying to buy relays with 'smart_enabled' set to false - should throw", async () => {
        const bntToken = await getEos(testUser).contract(networkToken);
        const minReturn = '0.0000000001';
        let throwed = false;
        try {
            await bntToken.transfer(
                {
                    from: testUser,
                    to: networkContract,
                    quantity: `5.0000000000 ${networkTokenSymbol}`,
                    memo: `1,${bntConverter} ${bntRelaySymbol},${minReturn},${testUser}`
                },
                { authorization: `${testUser}@active` }
            );
        }
        catch (e) {
            console.log(e)
            throwed = true;
        }

        throwed.should.be.equal(true);
    });
});