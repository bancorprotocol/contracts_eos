
# Group MultiConverter\_Reserves\_Table


[**Modules**](modules.md)
 **>** [**MultiConverter\_Reserves\_Table**](group___multi_converter___reserves___table.md)



_This table stores the reserve balances and related information for the reserves of every converter in this contract._ [More...](#detailed-description)














## Public Attributes

| Type | Name |
| ---: | :--- |
|  asset | [**balance**](group___multi_converter___reserves___table.md#variable-balance)  <br>_amount in the reserve_  |
|  name | [**contract**](group___multi_converter___reserves___table.md#variable-contract)  <br>_name of the account storing the token contract for this reserve's token_  |
|  uint64\_t | [**ratio**](group___multi_converter___reserves___table.md#variable-ratio)  <br>_reserve ratio relative to the other reserves_  |










# Detailed Description


SCOPE of this table is the converters' smart token symbol's `code().raw()` values 

    
## Public Attributes Documentation


### <a href="#variable-balance" id="variable-balance">variable balance </a>


```cpp
asset balance;
```


PRIMARY KEY for this table is `balance.symbol.code().raw()` 

        

### <a href="#variable-contract" id="variable-contract">variable contract </a>


```cpp
name contract;
```



### <a href="#variable-ratio" id="variable-ratio">variable ratio </a>


```cpp
uint64_t ratio;
```



------------------------------
