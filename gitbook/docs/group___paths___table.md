
# Group Paths\_Table


[**Modules**](modules.md)
 **>** [**Paths\_Table**](group___paths___table.md)



_This table stores the aggregate set of (output) reserves which can be purchased in exchange for a given (input) reserve._ [More...](#detailed-description)














## Public Attributes

| Type | Name |
| ---: | :--- |
|  symbol\_code | [**reserve**](group___paths___table.md#variable-reserve)  <br>_the reserve you can sell to buy_ `reserves` _above_ |
|  set&lt; symbol\_code &gt; | [**reserves**](group___paths___table.md#variable-reserves)  <br>_reserves you can buy in exchange for_ `reserve` _below_ |










# Detailed Description


SCOPE of this table is the MultiConverter contract (`_self`) 

    
## Public Attributes Documentation


### <a href="#variable-reserve" id="variable-reserve">variable reserve </a>


```cpp
symbol_code reserve;
```



### <a href="#variable-reserves" id="variable-reserves">variable reserves </a>


```cpp
set<symbol_code> reserves;
```



------------------------------
