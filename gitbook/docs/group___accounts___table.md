
# Group Accounts\_Table


[**Modules**](modules.md)
 **>** [**Accounts\_Table**](group___accounts___table.md)



_This table stores "temporary balances" that are transfered in by liquidity providers before they can get added to their respective reserves._ [More...](#detailed-description)














## Public Attributes

| Type | Name |
| ---: | :--- |
|  uint64\_t | [**id**](group___accounts___table.md#variable-id)  <br>_PRIMARY KEY for this table, unused dummy variable._  |
|  asset | [**quantity**](group___accounts___table.md#variable-quantity)  <br>_balance in the reserve currency_  |
|  symbol\_code | [**symbl**](group___accounts___table.md#variable-symbl)  <br>_symbol of the smart token (a way to reference converters) this balance pertains to_  |


## Public Functions

| Type | Name |
| ---: | :--- |
|  uint128\_t | [**by\_cnvrt**](group___accounts___table.md#function-by-cnvrt) () const<br>_SECONDARY KEY of this table based on the symbol of the temporary reserve balance for a particular converter._  |








# Detailed Description


SCOPE of this table is the `name.value` of the liquidity provider, owner of the `quantity` 

    
## Public Attributes Documentation


### <a href="#variable-id" id="variable-id">variable id </a>


```cpp
uint64_t id;
```



### <a href="#variable-quantity" id="variable-quantity">variable quantity </a>


```cpp
asset quantity;
```



### <a href="#variable-symbl" id="variable-symbl">variable symbl </a>


```cpp
symbol_code symbl;
```


## Public Functions Documentation


### <a href="#function-by-cnvrt" id="function-by-cnvrt">function by\_cnvrt </a>


```cpp
uint128_t by_cnvrt () const
```


uint128\_t { balance.symbol.code().raw() } &lt;&lt; 64 ) | currency.raw() 

        

------------------------------
