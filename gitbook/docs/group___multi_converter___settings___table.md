
# Group MultiConverter\_Settings\_Table


[**Modules**](modules.md)
 **>** [**MultiConverter\_Settings\_Table**](group___multi_converter___settings___table.md)



_This table stores the global settings affecting all the converters in this contract._ [More...](#detailed-description)














## Public Attributes

| Type | Name |
| ---: | :--- |
|  bool | [**active**](group___multi_converter___settings___table.md#variable-active)  <br>_toggle boolean to enable/disable all the converters in this contract_  |
|  uint64\_t | [**max\_fee**](group___multi_converter___settings___table.md#variable-max-fee)  <br>_maximum conversion fee for converters in this contract_  |
|  name | [**multi\_token**](group___multi_converter___settings___table.md#variable-multi-token)  <br>_account name of contract for relay tokens_  |
|  name | [**staking**](group___multi_converter___settings___table.md#variable-staking)  <br>_account name of contract for voting and staking_  |










# Detailed Description


Both SCOPE and PRIMARY KEY are `_self`, so this table is effectively a singleton 

    
## Public Attributes Documentation


### <a href="#variable-active" id="variable-active">variable active </a>


```cpp
bool active;
```



### <a href="#variable-max-fee" id="variable-max-fee">variable max\_fee </a>


```cpp
uint64_t max_fee;
```



### <a href="#variable-multi-token" id="variable-multi-token">variable multi\_token </a>


```cpp
name multi_token;
```



### <a href="#variable-staking" id="variable-staking">variable staking </a>


```cpp
name staking;
```



------------------------------
