
# Group BancorToken


[**Modules**](modules.md)
 **>** [**BancorToken**](group___bancor_token.md)



_BNT Token contract._ [More...](#detailed-description)









## Modules

| Type | Name |
| ---: | :--- |
| module | [**Currency Stats Table**](group___currency___stats___table.md) <br>_This table stores stats on the supply of the token governed by this contract._  |
| module | [**Accounts Table**](group___token___accounts___table.md) <br>_This table stores balances for every holder of this token._  |







## Public Functions

| Type | Name |
| ---: | :--- |
|  ACTION | [**close**](group___bancor_token.md#function-close) (name owner, symbol\_code symbol) <br>_Close action._  |
|  ACTION | [**create**](group___bancor_token.md#function-create) (name issuer, asset maximum\_supply) <br>_Create action._  |
|  ACTION | [**issue**](group___bancor_token.md#function-issue) (name to, asset quantity, string memo) <br>_Issue action._  |
|  ACTION | [**open**](group___bancor_token.md#function-open) (name owner, symbol\_code symbol, name ram\_payer) <br>_Open action._  |
|  ACTION | [**retire**](group___bancor_token.md#function-retire) (asset quantity, string memo) <br>_Retire action._  |
|  ACTION | [**transfer**](group___bancor_token.md#function-transfer) (name from, name to, asset quantity, string memo) <br>_Standard transfer action._  |
|  ACTION | [**transferbyid**](group___bancor_token.md#function-transferbyid) (name from, name to, name amount\_account, uint64\_t amount\_id, string memo) <br>_used for cross chain transfers_  |

## Public Static Functions

| Type | Name |
| ---: | :--- |
|  asset | [**get\_balance**](_token_8hpp.md#function-get-balance) (name token\_contract\_account, name owner, symbol\_code sym) <br>_Get balance method._  |
|  asset | [**get\_supply**](_token_8hpp.md#function-get-supply) (name token\_contract\_account, symbol\_code sym) <br>_Get supply method._  |







# Detailed Description


based on `eosio.token` contract (with addition of `transferbyid`) defines the structures and actions that allow to create, issue, and transfer BNT tokens 

    
## Public Functions Documentation


### <a href="#function-close" id="function-close">function close </a>


```cpp
ACTION close (
    name owner,
    symbol_code symbol
) 
```


This action is the opposite for open, it closes the account `owner` for token `symbol`. 

**Parameters:**


* owner - the owner account to execute the close action for, 
* symbol - the symbol of the token to execute the close action for. 



**Precondition:**

The pair of owner plus symbol has to exist otherwise no action is executed, 




**Precondition:**

If the pair of owner plus symbol exists, the balance has to be zero. 




        

### <a href="#function-create" id="function-create">function create </a>


```cpp
ACTION create (
    name issuer,
    asset maximum_supply
) 
```


Allows `issuer` account to create a token in supply of `maximum_supply`. If validation is successful a new entry in statstable for token symbol scope gets created. 

**Parameters:**


* issuer - the account that creates the token, 
* maximum\_supply - the maximum supply set for the token created. 



**Precondition:**

Token symbol has to be valid, 




**Precondition:**

Token symbol must not be already created, 




**Precondition:**

maximum\_supply has to be smaller than the maximum supply allowed by the system: 1^62 - 1. 




**Precondition:**

Maximum supply must be positive; 




        

### <a href="#function-issue" id="function-issue">function issue </a>


```cpp
ACTION issue (
    name to,
    asset quantity,
    string memo
) 
```


This action issues to `to` account a `quantity` of tokens. 

**Parameters:**


* to - the account to issue tokens to, it must be the same as the issuer, 
* quantity - the amount of tokens to be issued, 
* memo - the memo string that accompanies the token issue transaction. 



        

### <a href="#function-open" id="function-open">function open </a>


```cpp
ACTION open (
    name owner,
    symbol_code symbol,
    name ram_payer
) 
```


Allows `ram_payer` to create an account `owner` with zero balance for token `symbol` at the expense of `ram_payer`. 

**Parameters:**


* owner - the account to be created, 
* symbol - the token to be payed with by `ram_payer`, 
* ram\_payer - the account that supports the cost of this action. 



        

### <a href="#function-retire" id="function-retire">function retire </a>


```cpp
ACTION retire (
    asset quantity,
    string memo
) 
```


The opposite for create action, if all validations succeed, it debits the statstable.supply amount. 

**Parameters:**


* quantity - the quantity of tokens to retire, 
* memo - the memo string to accompany the transaction. 



        

### <a href="#function-transfer" id="function-transfer">function transfer </a>


```cpp
ACTION transfer (
    name from,
    name to,
    asset quantity,
    string memo
) 
```


Allows `from` account to transfer to `to` account the `quantity` tokens. One account is debited and the other is credited with quantity tokens. 

**Parameters:**


* from - the account to transfer from, 
* to - the account to be transferred to, 
* quantity - the quantity of tokens to be transferred, 
* memo - the memo string to accompany the transaction. 



        

### <a href="#function-transferbyid" id="function-transferbyid">function transferbyid </a>


```cpp
ACTION transferbyid (
    name from,
    name to,
    name amount_account,
    uint64_t amount_id,
    string memo
) 
```




**Parameters:**


* from - sender of the amount should match target 
* to - receiver of the amount 
* amount\_account - scope (target) of the transfer 
* amount\_id - id of the intended transfer, containing amount 
* memo - memo for the transfer 



        
## Public Static Functions Documentation


### <a href="#function-get-balance" id="function-get-balance">function get\_balance </a>


```cpp
static asset get_balance (
    name token_contract_account,
    name owner,
    symbol_code sym
) 
```


Get the balance for a token `sym_code` created by `token_contract_account` account, for account `owner`. 

**Parameters:**


* token\_contract\_account - the token creator account, 
* owner - the account for which the token balance is returned, 
* sym - the token for which the balance is returned. 



        

### <a href="#function-get-supply" id="function-get-supply">function get\_supply </a>


```cpp
static asset get_supply (
    name token_contract_account,
    symbol_code sym
) 
```


Gets the supply for token `sym_code`, created by `token_contract_account` account. 

**Parameters:**


* token\_contract\_account - the account to get the supply for, 
* sym - the symbol to get the supply for. 



        

------------------------------
