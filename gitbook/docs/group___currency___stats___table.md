
# Group Currency\_Stats\_Table


[**Modules**](modules.md)
 **>** [**Currency\_Stats\_Table**](group___currency___stats___table.md)



_This table stores stats on the supply of the token governed by this contract._ 














## Public Attributes

| Type | Name |
| ---: | :--- |
|  name | [**issuer**](group___currency___stats___table.md#variable-issuer)  <br>_Issuer of the token._  |
|  asset | [**max\_supply**](group___currency___stats___table.md#variable-max-supply)  <br>_Maximum supply of the token._  |
|  asset | [**supply**](group___currency___stats___table.md#variable-supply)  <br>_Current supply of the token._  |










## Public Attributes Documentation


### <a href="#variable-issuer" id="variable-issuer">variable issuer </a>


```cpp
name issuer;
```



### <a href="#variable-max-supply" id="variable-max-supply">variable max\_supply </a>


```cpp
asset max_supply;
```



### <a href="#variable-supply" id="variable-supply">variable supply </a>


```cpp
asset supply;
```


PRIMARY KEY of this table is `supply.symbol.code().raw()` 

        

------------------------------
