
const { JsSignatureProvider } = require('eosjs/dist/eosjs-jssig')
const { TextDecoder, TextEncoder } = require('util')
const { Api, JsonRpc } = require('eosjs')
const Decimal = require('decimal.js')
const fetch = require('node-fetch')
const Long  = require('long')
const chai = require('chai')
const assert = chai.assert
const fs = require('fs')

const ONE = new Decimal(1)
const MAX_RATIO = new Decimal(1000000)
const MAX_FEE = new Decimal(1000000)

// Keys associated with all test-related accounts
const EOS = "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3" //eosio
const eos = "5JUoVWoLLV3Sj7jUKmfE8Qdt7Eo7dUd4PGZ2snZ81xqgnZzGKdC" //eosio.token 
const bnt = "5JS9bTWMc52HWmMC8v58hdfePTxPV5dd5fcxq92xUzbfmafeeRo"
const con = "5KgKxmnm8oh5WbHC4jmLARNFdkkgVdZ389rdxwGEiBdAJHkubBH"
const rep = "5JFLPVygcZZdEno2WWWkf3fPriuxnvjtVpkThifYM5HwcKg6ndu"
const usr = "5JKAjH9WH4XnZCEe8v5Wir7awV4YBTVa8KUSqWJbQR6QGtj4yce"

// signatureProvider: Required for signing transactions, which we create using the user’s private key.
const signatureProvider = new JsSignatureProvider([EOS,eos,bnt,con,rep,usr])

// rpc: The rpc client which connects to the HTTP endpoint of an EOSIO node.
const rpc = new JsonRpc('http://127.0.0.1:8888', { fetch })

// api: The client we use to communicate with an EOS blockchain node and we initialize it using the Rpc and SignatureProvider we have created
const api = new Api({ rpc, signatureProvider, textDecoder: new TextDecoder(), textEncoder: new TextEncoder() })

function bytesToHex(bytes) {
    let leHex = ''
    for (const b of bytes) {
        const n = Number(b).toString(16);
        leHex += (n.length === 1 ? '0' : '') + n
    }
    return leHex
}
const charmap = '.12345abcdefghijklmnopqrstuvwxyz'
const charidx = ch => {
    const idx = charmap.indexOf(ch)
    if (idx === -1) 
        throw new TypeError(`Invalid character: '${ch}'`)
    return idx;
}
function randomAccountName() {
    const size = 12
    let text = ""
    for (let i=0; i<size; i++) text += charmap.charAt(Math.floor(Math.random() * charmap.length))
    return text
}
function symbolToValue(symbol) {
    if (typeof symbol !== 'string') 
        throw new TypeError('name parameter is a required string')
  
    if (symbol.length > 7) 
        throw new TypeError('A symbol can be up to 7 characters long')
    
    final = ""
    for (i = 0; i < symbol.length; i++) {
        code = symbol.charCodeAt(i)
        if (code < 65 || code > 90) 
            throw new TypeError('A symbol must have only A...Z caps characters')
        bin = Number(code).toString(2)
        while (bin.length < 8) bin = "0" + bin
        final = bin + final
    }
    return Long.fromString(final, true, 2)
}
function nameToValue(name) {
    if (typeof name !== 'string') 
        throw new TypeError('name parameter is a required string')
  
    if (name.length > 12) 
        throw new TypeError('A name can be up to 12 characters long')
  
    let bitstr = ''
    for (let i = 0; i <= 12; i++) {
      // process all 64 bits (even if name is short)
      const c = i < name.length ? charidx(name[i]) : 0
      const bitlen = i < 12 ? 5 : 4
      let bits = Number(c).toString(2)
      
      if (bits.length > bitlen)
        throw new TypeError('Invalid name ' + name)
      
      bits = '0'.repeat(bitlen - bits.length) + bits
      bitstr += bits
    }
    return Long.fromString(bitstr, true, 2)
}
function getTableBoundsForSymbol(symbol, asHex = true) {
    const symbolValue = symbolToValue(symbol)
    const symbolValueP1 = symbolValue.add(1)
    if (!asHex)
        return {
            lower_bound: symbolValue.toString(),
            upper_bound: symbolValueP1.toString()
        };
    const lowerBound = bytesToHex(symbolValue.toBytesLE())
    const upperBound = bytesToHex(symbolValueP1.toBytesLE())
    return {
        lower_bound: lowerBound,
        upper_bound: upperBound,
    }
}
function getTableBoundsForName(name, asHex = true) {
    const nameValue = nameToValue(name)
    const nameValueP1 = nameValue.add(1)
    if (!asHex)
        return {
            lower_bound: nameValue.toString(),
            upper_bound: nameValueP1.toString()
        };
    const lowerBound = bytesToHex(nameValue.toBytesLE())
    const upperBound = bytesToHex(nameValueP1.toBytesLE())
    return {
        lower_bound: lowerBound,
        upper_bound: upperBound,
    }
}
async function randomAccount(accnt = randomAccountName()) {
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
async function expectError(prom, expected_error='') {
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
async function expectNoError(prom) {
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
async function expectNoErrorPrint(prom) {
    var error = ''
    var result;
    try {
        result = await prom;
    }
    catch (err) {
        error = err
    }
    assert.equal(error, '', 'promise should have resolved');

    fs.writeFile('./temp.json', JSON.stringify(result, null, 2),'utf8' , function(err, data) {
        if (err) {
            console.log(err);
        } else {
            console.log("data written");
        }
    });
    return result
}
const snooze = ms => new Promise(resolve => setTimeout(resolve, ms))

const randomAmount = ({ min=0, max=100, decimals=8 }) => {
    return (Math.random() * (max - min) + min).toFixed(decimals)
}
const calculatePurchaseReturn = (supply, balance, ratio, amount, fee = 0) => {
    supply = Decimal(supply);
    balance = Decimal(balance);
    amount = Decimal(amount);
    ratio = Decimal(ratio);
    fee = Decimal(fee);
    const newAmount = supply.mul(
        ONE.add(amount.div(balance))
            .pow(ratio.div(MAX_RATIO))
            .sub(ONE)
    )
    return deductFee(newAmount, fee, 1);
}
const calculateSaleReturn = (supply, balance, ratio, amount, fee = 0) => {
    supply = Decimal(supply)
    balance = Decimal(balance)
    amount = Decimal(amount)
    ratio = Decimal(ratio)
    fee = Decimal(fee)
    
    const newAmount = balance.mul(
        ONE.sub(
            ONE.sub(amount.div(supply)).pow(MAX_RATIO.div(ratio))
        )
    )
    return deductFee(newAmount, fee, 1)
}
const deductFee = (amount, fee, magnitude) => {
    return amount.mul(
        ONE.minus(fee.div(MAX_FEE)).pow(magnitude)
    )
}
const extractEvents = async (conversionTx) => {
    if (conversionTx instanceof Promise)
        conversionTx = await conversionTx;
    
    const rawEvents = conversionTx
        .processed.action_traces[0].inline_traces[2].inline_traces[1].console
        .split("\n")
        .filter(Boolean)
        .map(JSON.parse)
    
    return {
        priceData: rawEvents.filter(({ etype }) => etype === 'price_data'),
        conversion: rawEvents.find(({ etype }) => etype === 'conversion')
    }
}


async function getTableRows(code, scope, table, key=null, limit=50, reverse=false) {
    return rpc.get_table_rows({
        code,
        scope,
        table,
        limit,
        reverse,
        show_payer: false,
        json: true,
        ...(key ? { key } : {})
    });
};


module.exports = {
    api, rpc,
    snooze,
    randomAmount,
    randomAccount,
    expectError,
    expectNoError,
    expectNoErrorPrint,
    getTableBoundsForName,
    getTableBoundsForSymbol,
    calculatePurchaseReturn,
    calculateSaleReturn,
    extractEvents,
    getTableRows
}
