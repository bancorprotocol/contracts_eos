
# Group Converter\_Settings\_Table


[**Modules**](modules.md)
 **>** [**Converter\_Settings\_Table**](group___converter___settings___table.md)



_This table stores stats on the settings of the converter._ [More...](#detailed-description)














## Public Attributes

| Type | Name |
| ---: | :--- |
|  bool | [**enabled**](group___converter___settings___table.md#variable-enabled)  <br>_true if conversions are enabled, false if not_  |
|  uint64\_t | [**fee**](group___converter___settings___table.md#variable-fee)  <br>_conversion fee for this converter_  |
|  uint64\_t | [**max\_fee**](group___converter___settings___table.md#variable-max-fee)  <br>_maximum conversion fee percentage, 0-30000, 4-pt precision a la eosio.asset_  |
|  name | [**network**](group___converter___settings___table.md#variable-network)  <br>_bancor network contract name_  |
|  bool | [**require\_balance**](group___converter___settings___table.md#variable-require-balance)  <br>_require creating new balance for the calling account should fail_  |
|  name | [**smart\_contract**](group___converter___settings___table.md#variable-smart-contract)  <br>_contract account name of the smart token governed by the converter_  |
|  asset | [**smart\_currency**](group___converter___settings___table.md#variable-smart-currency)  <br>_currency of the smart token governed by the converter_  |
|  bool | [**smart\_enabled**](group___converter___settings___table.md#variable-smart-enabled)  <br>_true if the smart token can be converted to/from, false if not_  |










# Detailed Description


Both SCOPE and PRIMARY KEY are `_self`, so this table is effectively a singleton. 

    
## Public Attributes Documentation


### <a href="#variable-enabled" id="variable-enabled">variable enabled </a>


```cpp
bool enabled;
```



### <a href="#variable-fee" id="variable-fee">variable fee </a>


```cpp
uint64_t fee;
```



### <a href="#variable-max-fee" id="variable-max-fee">variable max\_fee </a>


```cpp
uint64_t max_fee;
```



### <a href="#variable-network" id="variable-network">variable network </a>


```cpp
name network;
```



### <a href="#variable-require-balance" id="variable-require-balance">variable require\_balance </a>


```cpp
bool require_balance;
```



### <a href="#variable-smart-contract" id="variable-smart-contract">variable smart\_contract </a>


```cpp
name smart_contract;
```



### <a href="#variable-smart-currency" id="variable-smart-currency">variable smart\_currency </a>


```cpp
asset smart_currency;
```



### <a href="#variable-smart-enabled" id="variable-smart-enabled">variable smart\_enabled </a>


```cpp
bool smart_enabled;
```



------------------------------
