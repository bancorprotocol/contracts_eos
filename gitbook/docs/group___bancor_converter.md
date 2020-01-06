
# Group BancorConverter


[**Modules**](modules.md)
 **>** [**BancorConverter**](group___bancor_converter.md)



_Bancor Converter._ [More...](#detailed-description)









## Modules

| Type | Name |
| ---: | :--- |
| module | [**Reserves Table**](group___converter___reserves___table.md) <br>_This table stores stats on the reserves of the converter, the actual balance is owned by converter account within the accounts._  |
| module | [**Settings Table**](group___converter___settings___table.md) <br>_This table stores stats on the settings of the converter._  |







## Public Functions

| Type | Name |
| ---: | :--- |
|  ACTION | [**delreserve**](group___bancor_converter.md#function-delreserve) (symbol\_code currency) <br>_deletes an empty reserve_  |
|  ACTION | [**init**](group___bancor_converter.md#function-init) (name smart\_contract, asset smart\_currency, bool smart\_enabled, bool enabled, name network, bool require\_balance, uint64\_t max\_fee, uint64\_t fee) <br>_initializes the converter settings_  |
|  void | [**on\_transfer**](group___bancor_converter.md#function-on-transfer) (name from, name to, asset quantity, std::string memo) <br>_transfer intercepts_  |
|  ACTION | [**setreserve**](group___bancor_converter.md#function-setreserve) (name contract, symbol currency, uint64\_t ratio, bool sale\_enabled) <br>_initializes a new reserve in the converter_  |
|  ACTION | [**update**](group___bancor_converter.md#function-update) (bool smart\_enabled, bool enabled, bool require\_balance, uint64\_t fee) <br>_updates the converter settings_  |







## Macros

| Type | Name |
| ---: | :--- |
| define  | [**EMIT\_CONVERSION\_EVENT**](group___bancor_converter.md#define-emit-conversion-event) (memo, from\_contract, from\_symbol, to\_contract, to\_symbol, from\_amount, to\_amount, fee\_amount) <br>_triggered when a conversion between two tokens occurs_  |
| define  | [**EMIT\_CONVERSION\_FEE\_UPDATE\_EVENT**](group___bancor_converter.md#define-emit-conversion-fee-update-event) (prev\_fee, new\_fee) <br>_triggered when the conversion fee is updated_  |
| define  | [**EMIT\_PRICE\_DATA\_EVENT**](group___bancor_converter.md#define-emit-price-data-event) (smart\_supply, reserve\_contract, reserve\_symbol, reserve\_balance, reserve\_ratio) <br>_triggered after a conversion with new tokens price data_  |

# Detailed Description


The Bancor converter allows conversions between a smart token and tokens that are defined as its reserves and between the different reserves directly. 

    
## Public Functions Documentation


### <a href="#function-delreserve" id="function-delreserve">function delreserve </a>


```cpp
ACTION delreserve (
    symbol_code currency
) 
```




**Parameters:**


* `currency` - reserve token currency symbol 



        

### <a href="#function-init" id="function-init">function init </a>


```cpp
ACTION init (
    name smart_contract,
    asset smart_currency,
    bool smart_enabled,
    bool enabled,
    name network,
    bool require_balance,
    uint64_t max_fee,
    uint64_t fee
) 
```


can only be called once, by the contract account 

**Parameters:**


* `smart_contract` - contract account name of the smart token governed by the converter 
* `smart_currency` - currency of the smart token governed by the converter 
* `smart_enabled` - true if the smart token can be converted to/from, false if not 
* `enabled` - true if conversions are enabled, false if not 
* `require_balance` - true if conversions that require creating new balance for the calling account should fail, false if not 
* `network` - bancor network contract name 
* `max_fee` - maximum conversion fee percentage, 0-30000, 4-pt precision a la eosio.asset 
* `fee` - conversion fee percentage, must be lower than the maximum fee, same precision 



        

### <a href="#function-on-transfer" id="function-on-transfer">function on\_transfer </a>


```cpp
void on_transfer (
    name from,
    name to,
    asset quantity,
    std::string memo
) 
```


`memo` in csv format, may contain an extra keyword (e.g. "setup") following a semicolon at the end of the conversion path; indicates special transfer which otherwise would be interpreted as a standard conversion 

**Parameters:**


* `from` - the sender of the transfer 
* `to` - the receiver of the transfer 
* `quantity` - the quantity for the transfer 
* `memo` - the memo for the transfer 



        

### <a href="#function-setreserve" id="function-setreserve">function setreserve </a>


```cpp
ACTION setreserve (
    name contract,
    symbol currency,
    uint64_t ratio,
    bool sale_enabled
) 
```


can also be used to update an existing reserve, can only be called by the contract account 

**Parameters:**


* `contract` - reserve token contract name 
* `currency` - reserve token currency symbol 
* `ratio` - reserve ratio, percentage, 0-1000000, precision a la max\_fee 
* `sale_enabled` - true if purchases are enabled with the reserve, false if not 



        

### <a href="#function-update" id="function-update">function update </a>


```cpp
ACTION update (
    bool smart_enabled,
    bool enabled,
    bool require_balance,
    uint64_t fee
) 
```


can only be called by the contract account 

**Parameters:**


* `smart_enabled` - true if the smart token can be converted to/from, false if not 
* `enabled` - true if conversions are enabled, false if not 
* `require_balance` - true if conversions that require creating new balance for the calling account should fail, false if not 
* `fee` - conversion fee percentage, must be lower than the maximum fee, same precision 



        
## Macro Definition Documentation



### <a href="#define-emit-conversion-event" id="define-emit-conversion-event">define EMIT\_CONVERSION\_EVENT </a>


```cpp
#define EMIT_CONVERSION_EVENT (
    memo,
    from_contract,
    from_symbol,
    to_contract,
    to_symbol,
    from_amount,
    to_amount,
    fee_amount
) { \
    START_EVENT("conversion", "1.3") \
    EVENTKV("memo", memo) \
    EVENTKV("from_contract", from_contract) \
    EVENTKV("from_symbol", from_symbol) \
    EVENTKV("to_contract", to_contract) \
    EVENTKV("to_symbol", to_symbol) \
    EVENTKV("amount", from_amount) \
    EVENTKV("return", to_amount) \
    EVENTKVL("conversion_fee", fee_amount) \
    END_EVENT() \
}
```



### <a href="#define-emit-conversion-fee-update-event" id="define-emit-conversion-fee-update-event">define EMIT\_CONVERSION\_FEE\_UPDATE\_EVENT </a>


```cpp
#define EMIT_CONVERSION_FEE_UPDATE_EVENT (
    prev_fee,
    new_fee
) { \
    START_EVENT("conversion_fee_update", "1.1") \
    EVENTKV("prev_fee", prev_fee) \
    EVENTKVL("new_fee", new_fee) \
    END_EVENT() \
}
```



### <a href="#define-emit-price-data-event" id="define-emit-price-data-event">define EMIT\_PRICE\_DATA\_EVENT </a>


```cpp
#define EMIT_PRICE_DATA_EVENT (
    smart_supply,
    reserve_contract,
    reserve_symbol,
    reserve_balance,
    reserve_ratio
) { \
    START_EVENT("price_data", "1.4") \
    EVENTKV("smart_supply", smart_supply) \
    EVENTKV("reserve_contract", reserve_contract) \
    EVENTKV("reserve_symbol", reserve_symbol) \
    EVENTKV("reserve_balance", reserve_balance) \
    EVENTKVL("reserve_ratio", reserve_ratio) \
    END_EVENT() \
}
```



------------------------------
