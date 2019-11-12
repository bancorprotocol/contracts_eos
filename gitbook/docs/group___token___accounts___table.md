
# Group Token\_Accounts\_Table


[**Modules**](modules.md)
 **>** [**Token\_Accounts\_Table**](group___token___accounts___table.md)



_This table stores balances for every holder of this token._ [More...](#detailed-description)














## Public Attributes

| Type | Name |
| ---: | :--- |
|  asset | [**balance**](group___token___accounts___table.md#variable-balance)  <br>_User's balance balance in the token._  |










# Detailed Description


SCOPE is balance owner's `name.value` 

    
## Public Attributes Documentation


### <a href="#variable-balance" id="variable-balance">variable balance </a>


```cpp
asset balance;
```


PRIMARY KEY is `balance.symbol.code().raw()` 

        

------------------------------
