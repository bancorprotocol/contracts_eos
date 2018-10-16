var BancorNetwork = artifacts.require("./BancorNetwork/");
var BancorConverter = artifacts.require("./BancorConverter/");
var SmartToken = artifacts.require("./SmartToken/");

async function regConverter(deployer, network, accounts, token, symbol, networkContract, networkToken, networkTokenSymbol){
    const converter = await deployer.deploy(BancorConverter, `cnvt${token}`);

    const tknContract = await deployer.deploy(SmartToken, token);
    await tknContract.contractInstance.create({
      issuer: tknContract.contract.address,
      enabled: 1,
      maximum_supply: `1000000000.0000 ${symbol}`},
      {authorization: `${tknContract.contract.address}@active`,broadcast: true,sign: true});

    const tknrlyContract = await deployer.deploy(SmartToken, `tkn${networkToken.contract.address}${token}`);
    var rlySymbol = networkTokenSymbol + symbol;
    await tknrlyContract.contractInstance.create({
      issuer: converter.contract.address,
      enabled: 1,
      maximum_supply: `1000000000.0000 ${rlySymbol}`},
      {authorization: `${tknrlyContract.contract.address}@active`,broadcast: true,sign: true});

    
    // converter create
    await converter.contractInstance.create({
                        smart_contract: tknrlyContract.contract.address,
                        smart_currency: `0.0000 ${rlySymbol}`,
                        smart_enabled: 0,
                        enabled: 1,
                        network: networkContract.contract.address,
                        verifyram: 0
    },{authorization: `${converter.contract.address}@active`,broadcast: true,sign: true});        

    // converter setconnectors

    await converter.contractInstance.setconnector({
            contract:networkToken.contract.address,
            currency: `0.0000 ${networkTokenSymbol}`,
            weight: 500,
            enabled: 1,
            fee: 0,
            feeaccount: converter.contract.address,
            maxfee:0,
    },
        {authorization: `${converter.contract.address}@active`,broadcast: true,sign: true});
        
    await converter.contractInstance.setconnector({
            contract:tknContract.contract.address,
            currency: `0.0000 ${symbol}`,
            weight: 500,
            enabled: 1,
            fee: 0,
            feeaccount: converter.contract.address,
            maxfee:0,
        },{authorization: `${converter.contract.address}@active`,broadcast: true,sign: true});        
        
        
    // reg tokens
    await networkContract.contractInstance.regtoken({contract:tknContract.contract.address, enabled: 1},
        {authorization: `${networkContract.contract.address}@active`,broadcast: true,sign: true});
    await networkContract.contractInstance.regtoken({contract:tknrlyContract.contract.address, enabled: 1},
        {authorization: `${networkContract.contract.address}@active`,broadcast: true,sign: true});
        
    // reg converter
    await networkContract.contractInstance.regconverter({contract:converter.contract.address, enabled: 1},
        {authorization: `${networkContract.contract.address}@active`,broadcast: true,sign: true});

    // issue 3 tokens types to converter
    await tknContract.contractInstance.issue({
      to: converter.contract.address,
      quantity: `100000.0000 ${symbol}`,
      memo:"initial"
    },{authorization: `${tknContract.contract.address}@active`,broadcast: true,sign: true});
      
    await tknrlyContract.contractInstance.issue({
      to: converter.contract.address,
      quantity: `100000.0000 ${networkTokenSymbol + symbol}`,
      memo:"initial"  
    },{authorization: `${converter.contract.address}@active`,broadcast: true,sign: true, keyProvider: converter.keys.privateKey});
      
    await networkToken.contractInstance.issue({
      to: converter.contract.address,
      quantity: `100000.0000 ${networkTokenSymbol}`,
      memo:"initial"
    },{authorization: `${networkToken.contract.address}@active`,broadcast: true,sign: true});
}

module.exports = async function(deployer, network, accounts) {
  const networkContract = await deployer.deploy(BancorNetwork, "bancornetwrk");
  const tknbntContract = await deployer.deploy(SmartToken, "bnt");
  var networkTokenSymbol = "BNT";
  await tknbntContract.contractInstance.create({
      issuer: tknbntContract.contract.address,
      enabled: 1,
      maximum_supply: `1000000000.0000 ${networkTokenSymbol}`},
      {authorization: `${tknbntContract.contract.address}@active`,broadcast: true,sign: true});

  await networkContract.contractInstance.regtoken({contract:tknbntContract.contract.address, enabled: 1},
        {authorization: `${networkContract.contract.address}@active`,broadcast: true,sign: true});  
  
  for (var i = 0; i < tkns.length; i++) {
    const {contract, symbol} = tkns[i];
    await regConverter(deployer, network, accounts, contract, symbol, networkContract, tknbntContract, networkTokenSymbol);    
  }
  
  var test1User = 'test1';
  var test1Keys = await accounts.getCreateAccount('test1');
  await tknbntContract.contractInstance.issue({
      to: test1User,
      quantity: `100000.0000 ${networkTokenSymbol}`,
      memo:"test money"
    },{authorization: `${tknbntContract.contract.address}@active`,broadcast: true,sign: true});
  
};
var tkns = [];
tkns.push({contract:"aa", symbol:"TKNA"});
tkns.push({contract:"bb", symbol:"TKNB"});