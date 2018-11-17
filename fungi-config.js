module.exports = { 
    chains:{
        eos: {
          networks:{
            "development":{
              chainId: "",
              host: "127.0.0.1",
              port:8888,
              secured:false
            },
            "jungle":{
              chainId: "",
              host: "127.0.0.1",
              port:8888,
              secured:false
            },
            "kylin":{
              chainId: "5fff1dae8dc8e2fc4d5b23b2c7665c97f9e9d8edf2b6485a86ba311c25639191",
              host: "api.kylin.eoseco.com",
              port:80,
              secured:false
            },
            "mainnet":{
              chainId: "aca376f206b8fc25a6ed44dbdc66547c36c6c33e3a119ffbeaef943642f0e906",
              host: "node2.liquideos.com",
              port:80,
              secured:false
            }
          }
        }
    }
};