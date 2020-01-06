
# Group BancorXtransfer


[**Modules**](modules.md)
 **>** [**BancorXtransfer**](group___bancor_xtransfer.md)



_XTransferRerouter contract._ [More...](#detailed-description)









## Modules

| Type | Name |
| ---: | :--- |
| module | [**Settings Table**](group___xtransfer___settings___table.md) <br>_Basic minimal settings._  |







## Public Functions

| Type | Name |
| ---: | :--- |
|  ACTION | [**enablerrt**](group___bancor_xtransfer.md#function-enablerrt) (bool enable) <br>_can only be called by the contract account_  |
|  ACTION | [**reroutetx**](group___bancor_xtransfer.md#function-reroutetx) (uint64\_t tx\_id, string blockchain, string target) <br>_only the original sender may reroute an invalid transaction_  |







## Macros

| Type | Name |
| ---: | :--- |
| define  | [**EMIT\_TX\_REROUTE\_EVENT**](group___bancor_xtransfer.md#define-emit-tx-reroute-event) (tx\_id, blockchain, target) <br>_events triggered when an account reroutes an xtransfer transaction_  |

# Detailed Description


Allows rerouting transactions sent to BancorX with invalid parameters. 

    
## Public Functions Documentation


### <a href="#function-enablerrt" id="function-enablerrt">function enablerrt </a>


```cpp
ACTION enablerrt (
    bool enable
) 
```




**Parameters:**


* `enable` - true to enable rerouting xtransfers, false to disable it 



        

### <a href="#function-reroutetx" id="function-reroutetx">function reroutetx </a>


```cpp
ACTION reroutetx (
    uint64_t tx_id,
    string blockchain,
    string target
) 
```


allows an account to change xtransfer transaction details if the original transaction parameters were invalid (e.g non-existent destination blockchain/target) 

**Parameters:**


* `tx_id` - unique transaction id 
* `blockchain` - target blockchain 
* `target` - target account/address 



        
## Macro Definition Documentation



### <a href="#define-emit-tx-reroute-event" id="define-emit-tx-reroute-event">define EMIT\_TX\_REROUTE\_EVENT </a>


```cpp
#define EMIT_TX_REROUTE_EVENT (
    tx_id,
    blockchain,
    target
) START_EVENT("txreroute", "1.1") \
    EVENTKV("tx_id",tx_id) \
    EVENTKV("blockchain",blockchain) \
    EVENTKVL("target",target) \
    END_EVENT()
```



------------------------------
