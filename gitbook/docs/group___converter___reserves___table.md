
# Group Converter\_Reserves\_Table


[**Modules**](modules.md)
 **>** [**Converter\_Reserves\_Table**](group___converter___reserves___table.md)



_This table stores stats on the reserves of the converter, the actual balance is owned by converter account within the accounts._ [More...](#detailed-description)














## Public Attributes

| Type | Name |
| ---: | :--- |
|  name | [**contract**](group___converter___reserves___table.md#variable-contract)  <br>_Token contract for the currency._  |
|  asset | [**currency**](group___converter___reserves___table.md#variable-currency)  <br>_Symbol of the tokens in this reserve._  |
|  uint64\_t | [**ratio**](group___converter___reserves___table.md#variable-ratio)  <br>_Reserve ratio._  |
|  bool | [**sale\_enabled**](group___converter___reserves___table.md#variable-sale-enabled)  <br>_Are transactions enabled on this reserve._  |










# Detailed Description


SCOPE of this table is `_self` 

    
## Public Attributes Documentation


### <a href="#variable-contract" id="variable-contract">variable contract </a>


```cpp
name contract;
```



### <a href="#variable-currency" id="variable-currency">variable currency </a>


```cpp
asset currency;
```


PRIMARY KEY is `currency.symbol.code().raw()` 

        

### <a href="#variable-ratio" id="variable-ratio">variable ratio </a>


```cpp
uint64_t ratio;
```



### <a href="#variable-sale-enabled" id="variable-sale-enabled">variable sale\_enabled </a>


```cpp
bool sale_enabled;
```



------------------------------
