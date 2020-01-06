
# Group MultiConverter


[**Modules**](modules.md)
 **>** [**MultiConverter**](group___multi_converter.md)



_Bancor MultiConverter._ [More...](#detailed-description)









## Modules

| Type | Name |
| ---: | :--- |
| module | [**Accounts Table**](group___accounts___table.md) <br>_This table stores "temporary balances" that are transfered in by liquidity providers before they can get added to their respective reserves._  |
| module | [**Converters Table**](group___converters___table.md) <br>_This table stores the key information about all converters in this contract._  |
| module | [**Reserves Table**](group___multi_converter___reserves___table.md) <br>_This table stores the reserve balances and related information for the reserves of every converter in this contract._  |
| module | [**Settings Table**](group___multi_converter___settings___table.md) <br>_This table stores the global settings affecting all the converters in this contract._  |
| module | [**Paths Table**](group___paths___table.md) <br>_This table stores the aggregate set of (output) reserves which can be purchased in exchange for a given (input) reserve._  |







## Public Functions

| Type | Name |
| ---: | :--- |
|  ACTION | [**close**](group___multi_converter.md#function-close) (symbol\_code converter\_currency\_code) <br>_deletes a converter with empty reserves_  |
|  ACTION | [**create**](group___multi_converter.md#function-create) (name owner, symbol\_code token\_code, double initial\_supply) <br>_initializes a new converter_  |
|  ACTION | [**delreserve**](group___multi_converter.md#function-delreserve) (symbol\_code converter, symbol\_code currency) <br>_deletes an empty reserve in the converter_  |
|  ACTION | [**enablestake**](group___multi_converter.md#function-enablestake) (symbol\_code currency, bool enabled) <br>_flag indicating if the smart token can be staked, false if not_  |
|  ACTION | [**fund**](group___multi_converter.md#function-fund) (name sender, asset quantity) <br>_buys smart tokens with all connector tokens using the same percentage_  |
|  void | [**on\_transfer**](group___multi_converter.md#function-on-transfer) (name from, name to, asset quantity, string memo) <br>_transfer intercepts with standard transfer args_  |
|  ACTION | [**setmaxfee**](group___multi_converter.md#function-setmaxfee) (uint64\_t maxfee) <br>_modify maxfee in this multi-converter's settings_  |
|  ACTION | [**setmultitokn**](group___multi_converter.md#function-setmultitokn) (name multi\_token) <br>_creates the multi-converter settings, can only be called by multi-converter owner_  |
|  ACTION | [**setreserve**](group___multi_converter.md#function-setreserve) (symbol\_code converter\_currency\_code, symbol currency, name contract, uint64\_t ratio) <br>_initializes a new reserve in the converter_  |
|  ACTION | [**setstaking**](group___multi_converter.md#function-setstaking) (name staking) <br>_may only set staking/voting contract for this multi-converter once_  |
|  ACTION | [**updatefee**](group___multi_converter.md#function-updatefee) (symbol\_code currency, uint64\_t fee) <br>_updates the converter fee_  |
|  ACTION | [**updateowner**](group___multi_converter.md#function-updateowner) (symbol\_code currency, name new\_owner) <br>_change converter's owner_  |
|  ACTION | [**withdraw**](group___multi_converter.md#function-withdraw) (name sender, asset quantity, symbol\_code converter\_currency\_code) <br>_called by liquidity providers withdrawing "temporary balances" before_ `fund` _ing them into the reserve_ |







## Macros

| Type | Name |
| ---: | :--- |
| define  | [**EMIT\_CONVERSION\_EVENT**](group___multi_converter.md#define-emit-conversion-event) (converter\_currency\_symbol, memo, from\_contract, from\_symbol, to\_contract, to\_symbol, from\_amount, to\_amount, fee\_amount) <br>_triggered when a conversion between two tokens occurs_  |
| define  | [**EMIT\_CONVERSION\_FEE\_UPDATE\_EVENT**](group___multi_converter.md#define-emit-conversion-fee-update-event) (converter\_currency\_symbol, prev\_fee, new\_fee) <br>_triggered after a fee update occurs for a converter_  |
| define  | [**EMIT\_PRICE\_DATA\_EVENT**](group___multi_converter.md#define-emit-price-data-event) (converter\_currency\_symbol, smart\_supply, reserve\_contract, reserve\_symbol, reserve\_balance, reserve\_ratio) <br>_triggered after a conversion with new tokens price data_  |

# Detailed Description


The Bancor converter allows conversions between a smart token and tokens that are defined as its reserves and between the different reserves directly. 

    
## Public Functions Documentation


### <a href="#function-close" id="function-close">function close </a>


```cpp
ACTION close (
    symbol_code converter_currency_code
) 
```




**Parameters:**


* `converter_currency_code` - the currency code of the currency governed by the converter 



        

### <a href="#function-create" id="function-create">function create </a>


```cpp
ACTION create (
    name owner,
    symbol_code token_code,
    double initial_supply
) 
```




**Parameters:**


* `owner` - the converter creator 
* `initial_supply` - initial supply of the smart token governed by the converter 
* `maximum_supply` - maximum supply of the smart token governed by the converter 



        

### <a href="#function-delreserve" id="function-delreserve">function delreserve </a>


```cpp
ACTION delreserve (
    symbol_code converter,
    symbol_code currency
) 
```




**Parameters:**


* `converter` - the currency code of the smart token governed by the converter 
* `currency` - reserve token currency code 



        

### <a href="#function-enablestake" id="function-enablestake">function enablestake </a>


```cpp
ACTION enablestake (
    symbol_code currency,
    bool enabled
) 
```




**Parameters:**


* `currency` - the currency symbol governed by the converter 
* `enabled` - true if staking/voting for this symbol are enabled 



        

### <a href="#function-fund" id="function-fund">function fund </a>


```cpp
ACTION fund (
    name sender,
    asset quantity
) 
```


i.e. if the caller increases the supply by 10%, it will cost an amount equal to 10% of each connector token balance can only be called if the total ratio is exactly 100% 

**Parameters:**


* `sender` - sender of the quantity 
* `quantity` - amount to increase the supply by (in the smart token) 



        

### <a href="#function-on-transfer" id="function-on-transfer">function on\_transfer </a>


```cpp
void on_transfer (
    name from,
    name to,
    asset quantity,
    string memo
) 
```


`memo` containing a keyword following a semicolon at the end of the conversion path indicates special kind of transfer:
* e.g. transferring smart tokens with keyword "liquidate", or
* transferring reserve tokens with keyword "fund" 

**Parameters:**


  * `from` - the sender of the transfer 
  * `to` - the receiver of the transfer 
  * `quantity` - the quantity for the transfer 
  * `memo` - the memo for the transfer 





        

### <a href="#function-setmaxfee" id="function-setmaxfee">function setmaxfee </a>


```cpp
ACTION setmaxfee (
    uint64_t maxfee
) 
```




**Parameters:**


* `maxfee` - maximum fee for all converters in this multi-converter 



        

### <a href="#function-setmultitokn" id="function-setmultitokn">function setmultitokn </a>


```cpp
ACTION setmultitokn (
    name multi_token
) 
```




**Parameters:**


* `multi_token` - may only set multi-token contract once 



        

### <a href="#function-setreserve" id="function-setreserve">function setreserve </a>


```cpp
ACTION setreserve (
    symbol_code converter_currency_code,
    symbol currency,
    name contract,
    uint64_t ratio
) 
```


can also be used to update an existing reserve, can only be called by the contract account 

**Parameters:**


* `converter_currency_code` - the currency code of the currency governed by the converter 
* `currency` - reserve token currency symbol 
* `contract` - reserve token contract name 
* `ratio` - reserve ratio, percentage, 0-1000000 



        

### <a href="#function-setstaking" id="function-setstaking">function setstaking </a>


```cpp
ACTION setstaking (
    name staking
) 
```




**Parameters:**


* `staking` - name of staking/voting contract 



        

### <a href="#function-updatefee" id="function-updatefee">function updatefee </a>


```cpp
ACTION updatefee (
    symbol_code currency,
    uint64_t fee
) 
```




**Parameters:**


* `currency` - the currency symbol governed by the converter 
* `fee` - the new fee % for this converter, must be lower than the maximum fee, 0-1000000 



        

### <a href="#function-updateowner" id="function-updateowner">function updateowner </a>


```cpp
ACTION updateowner (
    symbol_code currency,
    name new_owner
) 
```




**Parameters:**


* `currency` - the currency symbol governed by the converter 
* `new_owner` - converter's new owner 



        

### <a href="#function-withdraw" id="function-withdraw">function withdraw </a>


```cpp
ACTION withdraw (
    name sender,
    asset quantity,
    symbol_code converter_currency_code
) 
```




**Parameters:**


* `sender` - sender of the quantity 
* `quantity` - amount to decrease the supply by (in the smart token) 
* `converter_currency_code` - the currency code of the currency governed by the converter 



        
## Macro Definition Documentation



### <a href="#define-emit-conversion-event" id="define-emit-conversion-event">define EMIT\_CONVERSION\_EVENT </a>


```cpp
#define EMIT_CONVERSION_EVENT (
    converter_currency_symbol,
    memo,
    from_contract,
    from_symbol,
    to_contract,
    to_symbol,
    from_amount,
    to_amount,
    fee_amount
) { \
    START_EVENT("conversion", "1.4") \
    EVENTKV("converter_currency_symbol", converter_currency_symbol) \
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
    converter_currency_symbol,
    prev_fee,
    new_fee
) { \
    START_EVENT("conversion_fee_update", "1.2") \
    EVENTKV("converter_currency_symbol", converter_currency_symbol) \
    EVENTKV("prev_fee", prev_fee) \
    EVENTKVL("new_fee", new_fee) \
    END_EVENT() \
}
```



### <a href="#define-emit-price-data-event" id="define-emit-price-data-event">define EMIT\_PRICE\_DATA\_EVENT </a>


```cpp
#define EMIT_PRICE_DATA_EVENT (
    converter_currency_symbol,
    smart_supply,
    reserve_contract,
    reserve_symbol,
    reserve_balance,
    reserve_ratio
) { \
    START_EVENT("price_data", "1.5") \
    EVENTKV("converter_currency_symbol", converter_currency_symbol) \
    EVENTKV("smart_supply", smart_supply) \
    EVENTKV("reserve_contract", reserve_contract) \
    EVENTKV("reserve_symbol", reserve_symbol) \
    EVENTKV("reserve_balance", reserve_balance) \
    EVENTKVL("reserve_ratio", reserve_ratio) \
    END_EVENT() \
}
```



------------------------------
