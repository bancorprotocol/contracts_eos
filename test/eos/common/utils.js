const { JsSignatureProvider } = require('eosjs/dist/eosjs-jssig'); 
const { TextDecoder, TextEncoder } = require('util'); 
const { Api, JsonRpc } = require('eosjs');
const fetch = require('node-fetch');                  
const chai = require('chai')
var assert = chai.assert;

// Keys associated with all test-related accounts
const EOS = "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"; //eosio
const eos = "5JUoVWoLLV3Sj7jUKmfE8Qdt7Eo7dUd4PGZ2snZ81xqgnZzGKdC"; //eosio.token 
const bnt = "5JS9bTWMc52HWmMC8v58hdfePTxPV5dd5fcxq92xUzbfmafeeRo";
const con = "5KgKxmnm8oh5WbHC4jmLARNFdkkgVdZ389rdxwGEiBdAJHkubBH";
const rep = "5JFLPVygcZZdEno2WWWkf3fPriuxnvjtVpkThifYM5HwcKg6ndu";
const usr = "5JKAjH9WH4XnZCEe8v5Wir7awV4YBTVa8KUSqWJbQR6QGtj4yce";

// signatureProvider: Required for signing transactions, which we create using the user’s private key.
const signatureProvider = new JsSignatureProvider([EOS,eos,bnt,con,rep,usr]);

// rpc: The rpc client which connects to the HTTP endpoint of an EOSIO node.
const rpc = new JsonRpc('http://127.0.0.1:8888', { fetch });

// api: The client we use to communicate with an EOS blockchain node and we initialize it using the Rpc and SignatureProvider we have created
const api = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() });

function randomAccountName(){
    const size = 12;
    let text = "";
    const possible = "abcdefghijklmnopqrstuvwxyz12345";
    for(let i=0; i<size; i++) text += possible.charAt(Math.floor(Math.random() * possible.length));
    return text;
}

async function createEOSaccount(accnt = randomAccountName()) {
    try {
        const result = await api.transact({
            actions: [{
                account: "eosio",
                name: "newaccount",
                authorization: [{
                    actor: "eosio",
                    permission: "active"
                }],
                data: {
                  creator: "eosio",
                  name: accnt,
                  owner: {
                    threshold: 1,
                    keys: [{
                        key: usr,
                        weight: 1
                    }],
                    accounts: [],
                    waits: []
                  },
                  active: {
                    threshold: 1,
                    keys: [{
                        key:usr,
                        weight: 1
                    }],
                    accounts: [],
                    waits: []
                  }
                }
              }]
          },
          {
            blocksBehind: 3,
            expireSeconds: 30
          })
        return result
    } catch(err) {
        throw(err)
    }
}

async function expectError(prom, expected_error='') 
{
    try {
        await prom;
        assert(false, 'should have failed');
    }
    catch (err) {
        if (expected_error !== '')
            assert.include(err.message, expected_error, 'valid error')
        else
            console.log(err.message)
    }
}
async function expectNoError(prom) 
{
    var error = ''
    var result;
    try {
        result = await prom;
    }
    catch (err) {
        error = err
    }
    assert.equal(error, '', 'promise should have resolved');
    return result
}
const snooze = ms => new Promise(resolve => setTimeout(resolve, ms));

module.exports = {
    api,
    rpc,
    snooze,
    expectError,
    expectNoError,
    createEOSaccount
}
