var SmartToken = artifacts.require("./SmartToken/");
var BancorNetwork = artifacts.require("./BancorNetwork/");
var BancorConverter = artifacts.require("./BancorConverter/");

async function regConverter(deployer, token, symbol, networkContract, networkToken, networkTokenSymbol) {
    const converter = await deployer.deploy(BancorConverter, `cnvt${token}`);

    const tknContract = await deployer.deploy(SmartToken, token);
    await tknContract.contractInstance.create({
        issuer: tknContract.contract.address,
        maximum_supply: `1000000000.0000 ${symbol}`},
        { authorization: `${tknContract.contract.address}@active`, broadcast: true, sign: true });

    const tknrlyContract = await deployer.deploy(SmartToken, `tkn${networkToken.contract.address}${token}`);
    var rlySymbol = networkTokenSymbol + symbol;
    await tknrlyContract.contractInstance.create({
        issuer: converter.contract.address,
        maximum_supply: `1000000000.0000 ${rlySymbol}`},
        { authorization: `${tknrlyContract.contract.address}@active`, broadcast: true, sign: true });

    // converter create
    await converter.contractInstance.init({
        smart_contract: tknrlyContract.contract.address,
        smart_currency: `0.0000 ${rlySymbol}`,
        smart_enabled: 0,
        enabled: 1,
        network: networkContract.contract.address,
        verify_ram: 0,
        max_fee: 0,
        fee: 0
    }, { authorization: `${converter.contract.address}@active`, broadcast: true, sign: true });        

    // converter set reserves
    await converter.contractInstance.setreserve({
        contract:networkToken.contract.address,
        currency: `0.0000 ${networkTokenSymbol}`,
        ratio: 500,
        p_enabled: 1
    }, { authorization: `${converter.contract.address}@active`, broadcast: true, sign: true });
        
    await converter.contractInstance.setreserve({
        contract:tknContract.contract.address,
        currency: `0.0000 ${symbol}`,
        ratio: 500,
        p_enabled: 1
    }, { authorization: `${converter.contract.address}@active`, broadcast: true, sign: true });

    // issue 3 tokens types to converter
    await tknContract.contractInstance.issue({
        to: converter.contract.address,
        quantity: `100000.0000 ${symbol}`,
        memo: "setup"
    }, { authorization: `${tknContract.contract.address}@active`, broadcast: true, sign: true });
      
    await tknrlyContract.contractInstance.issue({
        to: converter.contract.address,
        quantity: `100000.0000 ${networkTokenSymbol + symbol}`,
        memo: "setup"  
    }, { authorization: `${converter.contract.address}@active`, broadcast: true, sign: true, keyProvider: converter.keys.privateKey });
      
    await networkToken.contractInstance.issue({
        to: converter.contract.address,
        quantity: `100000.0000 ${networkTokenSymbol}`,
        memo: "setup"
    }, { authorization: `${networkToken.contract.address}@active`, broadcast: true, sign: true });
}

module.exports = async function(deployer, network, accounts) {
    const networkContract = await deployer.deploy(BancorNetwork, "bancornetwrk");
    const tknbntContract = await deployer.deploy(SmartToken, "bnt");
    var networkTokenSymbol = "BNT";
    await tknbntContract.contractInstance.create({
        issuer: tknbntContract.contract.address,
        maximum_supply: `1000000000.0000 ${networkTokenSymbol}`},
        { authorization: `${tknbntContract.contract.address}@active`, broadcast: true, sign: true });

    for (var i = 0; i < tkns.length; i++) {
        const { contract, symbol } = tkns[i];
        await regConverter(deployer, contract, symbol, networkContract, tknbntContract, networkTokenSymbol);    
    }
  
    var test1User = 'test1';
    var test1Keys = await accounts.getCreateAccount('test1');
    await tknbntContract.contractInstance.issue({
        to: test1User,
        quantity: `100000.0000 ${networkTokenSymbol}`,
        memo: "test money"
    }, { authorization: `${tknbntContract.contract.address}@active`, broadcast: true, sign: true });
};

var tkns = [];
tkns.push({ contract: "aa", symbol: "TKNA" });
tkns.push({ contract: "bb", symbol: "TKNB" });
