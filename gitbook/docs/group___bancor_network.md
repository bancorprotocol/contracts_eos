
# Group BancorNetwork


[**Modules**](modules.md)
 **>** [**BancorNetwork**](group___bancor_network.md)



_The BancorNetwork contract is the main entry point for Bancor token conversions._ [More...](#detailed-description)
















## Public Functions

| Type | Name |
| ---: | :--- |
|  ACTION | [**init**](group___bancor_network.md#function-init) () <br> |
|  void | [**on\_transfer**](group___bancor_network.md#function-on-transfer) (name from, name to, asset quantity, string memo) <br>_transfer intercepts_  |








# Detailed Description


This contract allows converting between the 'from' token being transfered into the contract and any other token in the Bancor network, initiated by submitting a single transaction and providing into `on_transfer` a "conversion path" in the `memo`:
* The path is most useful when the conversion must be routed through multiple "hops".
* The path defines which converters should be used and what kind of conversion should be done in each "hop".
* The path format doesn't include complex structure; it is represented by a space-delimited list of values in which each 'hop' is represented by a pair  converter s& 'to' token symbol. Here is the format: 
> [converter account, to token symbol, converter account, to token symbol...] 

* For example, in order to convert 10 EOS into BNT, the caller needs to transfer 10 EOS to the contract and provide the following memo: 
> `1,bnt2eoscnvrt BNT,1.0000000000,receiver_account_name`




    
## Public Functions Documentation


### <a href="#function-init" id="function-init">function init </a>


```cpp
ACTION init () 
```



### <a href="#function-on-transfer" id="function-on-transfer">function on\_transfer </a>


```cpp
void on_transfer (
    name from,
    name to,
    asset quantity,
    string memo
) 
```


conversion will fail if the amount returned is lower "minreturn" element in the `memo` 

        

------------------------------
