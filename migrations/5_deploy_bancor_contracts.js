var Token = artifacts.require("./Token/");
var BancorX = artifacts.require("./BancorX/");
var BancorNetwork = artifacts.require("./BancorNetwork/");
var BancorConverter = artifacts.require("./BancorConverter/");
var XTransferRerouter = artifacts.require("./XTransferRerouter/");

let networkContract;

async function regConverter(deployer, token, symbol, fee, networkContract, networkToken, networkTokenSymbol, issuerAccount, issuerPrivateKey, enableConverter=true) {
    const converter = await deployer.deploy(BancorConverter, `cnvt${token}`);

    const tknContract = await deployer.deploy(Token, token);
    await tknContract.contractInstance.create({
        issuer: tknContract.contract.address,
        maximum_supply: `1000000000.00000000 ${symbol}`},
        { authorization: `${tknContract.contract.address}@active`, broadcast: true, sign: true });

    const tknrlyContract = await deployer.deploy(Token, `tkn${networkToken.contract.address}${token}`);
    var rlySymbol = networkTokenSymbol + symbol;
    await tknrlyContract.contractInstance.create({
        issuer: converter.contract.address,
        maximum_supply: `250000000.0000000000 ${rlySymbol}`},
        { authorization: `${tknrlyContract.contract.address}@active`, broadcast: true, sign: true });

    await converter.contractInstance.init({
        smart_contract: tknrlyContract.contract.address,
        smart_currency: `0.0000000000 ${rlySymbol}`,
        smart_enabled: 1,
        enabled: enableConverter ? 1 : 0,
        network: networkContract.contract.address,
        require_balance: 0,
        max_fee: 30,
        fee
    }, { authorization: `${converter.contract.address}@active`, broadcast: true, sign: true });        

    await converter.contractInstance.setreserve({
        contract:networkToken.contract.address,
        currency: `0.0000000000 ${networkTokenSymbol}`,
        ratio: 500,
        p_enabled: 1
    }, { authorization: `${converter.contract.address}@active`, broadcast: true, sign: true });
        
    await converter.contractInstance.setreserve({
        contract:tknContract.contract.address,
        currency: `0.00000000 ${symbol}`,
        ratio: 500,
        p_enabled: 1
    }, { authorization: `${converter.contract.address}@active`, broadcast: true, sign: true });

    await tknContract.contractInstance.issue({
        to: converter.contract.address,
        quantity: `100000.00000000 ${symbol}`,
        memo: "setup"
    }, { authorization: `${tknContract.contract.address}@active`, broadcast: true, sign: true });
      
    await tknrlyContract.contractInstance.issue({
        to: converter.contract.address,
        quantity: `100000.0000000000 ${networkTokenSymbol + symbol}`,
        memo: "setup"  
    }, { authorization: `${converter.contract.address}@active`, broadcast: true, sign: true, keyProvider: converter.keys.privateKey });
    await networkToken.contractInstance.issue({
        to: converter.contract.address,
        quantity: `100000.0000000000 ${networkTokenSymbol}`,
        memo: "setup"
    }, { authorization: `${issuerAccount}@active`, broadcast: true, sign: true, keyProvider: issuerPrivateKey });
}

module.exports = async function(deployer, network, accounts) {
    const bancorxContract = await deployer.deploy(BancorX, "bancorxoneos");
    
    networkContract = await deployer.deploy(BancorNetwork, "thisisbancor");
    const tknbntContract = await deployer.deploy(Token, "bnt");
    await deployer.deploy(XTransferRerouter, "txrerouter");

    const converter = await deployer.deploy(BancorConverter, "bnt2eoscnvrt")
    
    const bntrlyContract = await deployer.deploy(Token, "bnt2eosrelay");

    var networkTokenSymbol = "BNT";

    // create BNT
    await tknbntContract.contractInstance.create({
        issuer: bancorxContract.contract.address,
        maximum_supply: `250000000.0000000000 ${networkTokenSymbol}`},
        { authorization: `${tknbntContract.contract.address}@active`, broadcast: true, sign: true });


    // create BNTEOS
    await bntrlyContract.contractInstance.create({
        issuer: converter.account,
        maximum_supply: `250000000.0000000000 BNTEOS`
    }, {
        authorization: `${bntrlyContract.account}@active`
    });

    // create BNTEOS converter
    await converter.contractInstance.init({
        smart_contract: bntrlyContract.account,
        smart_currency: `0.0000000000 BNTEOS`,
        smart_enabled: 0,
        enabled: 1,
        network: networkContract.account,
        require_balance: 0,
        max_fee: 0,
        fee: 0
    }, {
        authorization: `${converter.account}@active`
    });

    // set BNT as reserve
    await converter.contractInstance.setreserve({
        contract: tknbntContract.contract.address,
        currency: `0.0000000000 BNT`,
        ratio: 500,
        p_enabled: 1
    }, {
        authorization: `${converter.account}@active`
    });

    // set SYS as reserve
    await converter.contractInstance.setreserve({
        contract: "eosio.token",
        currency: `0.0000 SYS`,
        ratio: 500,
        p_enabled: 1
    }, {
        authorization: `${converter.account}@active`
    });

    // send SYS to converter
    const eosioToken = await bancorxContract.eos.contract('eosio.token')    
    await eosioToken.transfer({
        from: 'eosio',
        to: converter.account,
        quantity: `10000.0000 SYS`,
        memo: 'setup'
    }, {
        authorization: 'eosio@active',
        keyProvider: '5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3'
    })

    // issue BNT to converter
    await tknbntContract.contractInstance.issue({
        to: converter.account,
        quantity: `90000.0000000000 BNT`,
        memo: 'setup'
    }, {
        authorization: `${bancorxContract.account}@active`,
        keyProvider: bancorxContract.keys.privateKey
    });

    // issue BNTEOS
    await bntrlyContract.contractInstance.issue({
        to: converter.account,
        quantity: `20000.0000000000 BNTEOS`,
        memo: 'setup'
    }, {
        authorization: `${converter.account}@active`,
        keyProvider: converter.keys.privateKey
    });


    // initialize bancorx
    await bancorxContract.contractInstance.init({
        x_token_name: tknbntContract.contract.address,
        min_reporters: 2,
        min_limit: 1,
        limit_inc: 100000000000000,
        max_issue_limit: 10000000000000000,
        max_destroy_limit: 10000000000000000},
        {authorization: `${bancorxContract.contract.address}@active`,broadcast: true,sign: true});

    await accounts.getCreateAccount('reporter1');
    await accounts.getCreateAccount('reporter2');
    await accounts.getCreateAccount('reporter3');
    await accounts.getCreateAccount('reporter4');
    await accounts.getCreateAccount('test1');

    await bancorxContract.contractInstance.addreporter({
        reporter: 'reporter1'},
        {authorization: `${bancorxContract.contract.address}@active`,broadcast: true,sign: true});

    await bancorxContract.contractInstance.addreporter({
        reporter: 'reporter2'},
        {authorization: `${bancorxContract.contract.address}@active`,broadcast: true,sign: true});

    await bancorxContract.contractInstance.addreporter({
        reporter: 'reporter3'},
        {authorization: `${bancorxContract.contract.address}@active`,broadcast: true,sign: true});

    await bancorxContract.contractInstance.enablext({
        enable: 1},
        {authorization: `${bancorxContract.contract.address}@active`,broadcast: true,sign: true});

    await bancorxContract.contractInstance.enablerpt({
        enable: 1},
        {authorization: `${bancorxContract.contract.address}@active`,broadcast: true,sign: true});

    var contract1 = await bancorxContract.eos.contract(tknbntContract.contract.address);
    await contract1.issue({
        to: 'test1',
        quantity: `10000.0000000000 ${networkTokenSymbol}`,
        memo: "test money"
    }, { authorization: `${bancorxContract.contract.address}@active`, broadcast: true, sign: true });

    contract1.issue({
        to: 'reporter1',
        quantity: `100.0000000000 ${networkTokenSymbol}`,
        memo: "test money"
        },{authorization: `${bancorxContract.contract.address}@active`,broadcast: true,sign: true});

    for (var i = 0; i < tkns.length; i++) {
        const { contract, symbol, fee, enableConverter } = tkns[i];
        await regConverter(deployer, contract, symbol, fee, networkContract, tknbntContract, networkTokenSymbol, bancorxContract.contract.address, bancorxContract.keys.privateKey, enableConverter);    
    }
};

var tkns = [];
tkns.push({ contract: "aa", symbol: "TKNA", fee: 0, enableConverter: true });
tkns.push({ contract: "bb", symbol: "TKNB", fee: 1, enableConverter: true });
tkns.push({ contract: "cc", symbol: "TKNC", fee: 0, enableConverter: true });
tkns.push({ contract: "dd", symbol: "TKND", fee: 0, enableConverter: false });
