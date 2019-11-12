
# Group Converters\_Table


[**Modules**](modules.md)
 **>** [**Converters\_Table**](group___converters___table.md)



_This table stores the key information about all converters in this contract._ [More...](#detailed-description)














## Public Attributes

| Type | Name |
| ---: | :--- |
|  symbol | [**currency**](group___converters___table.md#variable-currency)  <br>_symbol of the smart token_  _representing a share in the reserves of this converter_ |
|  bool | [**enabled**](group___converters___table.md#variable-enabled)  <br>_toggle boolean to enable/disable this converter_  |
|  uint64\_t | [**fee**](group___converters___table.md#variable-fee)  <br>_conversion fee for this converter, applied on every hop_  |
|  bool | [**launched**](group___converters___table.md#variable-launched)  <br>_Has this converter launched (ever been enabled)_  |
|  name | [**owner**](group___converters___table.md#variable-owner)  <br>_creator of the converter_  |
|  bool | [**stake\_enabled**](group___converters___table.md#variable-stake-enabled)  <br>_toggle boolean to enable/disable this staking and voting for this converter_  |










# Detailed Description


SCOPE of this table is the converters' smart token symbol's `code().raw()` values 

    
## Public Attributes Documentation


### <a href="#variable-currency" id="variable-currency">variable currency </a>


```cpp
symbol currency;
```


PRIMARY KEY for this table is `currency.code().raw()` 

        

### <a href="#variable-enabled" id="variable-enabled">variable enabled </a>


```cpp
bool enabled;
```



### <a href="#variable-fee" id="variable-fee">variable fee </a>


```cpp
uint64_t fee;
```



### <a href="#variable-launched" id="variable-launched">variable launched </a>


```cpp
bool launched;
```



### <a href="#variable-owner" id="variable-owner">variable owner </a>


```cpp
name owner;
```



### <a href="#variable-stake-enabled" id="variable-stake-enabled">variable stake\_enabled </a>


```cpp
bool stake_enabled;
```



------------------------------
