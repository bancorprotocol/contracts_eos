
/**
 *  @file
 *  @copyright defined in ../../../LICENSE
 */
#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <string>

using namespace eosio;
using std::string;

/**
 * @defgroup BancorToken Token
 * @brief BNT Token contract
 * @details based on `eosio.token` contract (with addition of `transferbyid`) defines the 
 * structures and actions that allow to create, issue, and transfer BNT tokens
 * @{
 */

/*! \cond DOCS_EXCLUDE */
CONTRACT Token : public contract { /*! \endcond */
    public:
        using contract::contract;

        /*! \cond DOCS_EXCLUDE */
        struct transfer_args {
            name    from;
            name    to;
            asset   quantity;
            string  memo;
        }; /*! \endcond */

        /** 
         * @defgroup Token_Accounts_Table Accounts Table 
         * @brief This table stores balances for every holder of this token
         * @details SCOPE is balance owner's `name.value`
         * @{
         *//*! \cond DOCS_EXCLUDE */
            TABLE account { /*! \endcond */
                /**
                 * @brief User's balance balance in the token 
                 * @details PRIMARY KEY is `balance.symbol.code().raw()`
                 */
                asset balance;
                
                /*! \cond DOCS_EXCLUDE */
                uint64_t primary_key() const { return balance.symbol.code().raw(); }
                /*! \endcond */

            }; /** @}*/

        /** 
         * @defgroup Currency_Stats_Table Currency Stats Table 
         * @brief This table stores stats on the supply of the token governed by this contract 
         * @{
         *//*! \cond DOCS_EXCLUDE */
            TABLE currency_stats { /*! \endcond */                
                /**
                 * @brief Current supply of the token
                 * @details PRIMARY KEY of this table is `supply.symbol.code().raw()`
                 */
                asset   supply;
                
                /**
                 * @brief Maximum supply of the token
                 */
                asset   max_supply;

                /**
                 * @brief Issuer of the token
                 */
                name    issuer;
                
                /*! \cond DOCS_EXCLUDE */
                uint64_t primary_key() const { return supply.symbol.code().raw(); }
                /*! \endcond */

            }; /** @}*/

        /*! \cond DOCS_EXCLUDE */
        typedef eosio::multi_index<"accounts"_n, account> accounts;
        typedef eosio::multi_index<"stat"_n, currency_stats> stats;
        /*! \endcond */

        /**
         * @brief Create action.
         * @details Allows `issuer` account to create a token in supply of `maximum_supply`.
         * If validation is successful a new entry in statstable for token symbol scope gets created.
         * @param issuer - the account that creates the token,
         * @param maximum_supply - the maximum supply set for the token created.
         * @pre Token symbol has to be valid,
         * @pre Token symbol must not be already created,
         * @pre maximum_supply has to be smaller than the maximum supply allowed by the system: 1^62 - 1.
         * @pre Maximum supply must be positive;
         */
        ACTION create(name issuer, asset maximum_supply);

        /**
         * @brief Issue action.
         * @details This action issues to `to` account a `quantity` of tokens.
         * @param to - the account to issue tokens to, it must be the same as the issuer,
         * @param quantity - the amount of tokens to be issued,
         * @param memo - the memo string that accompanies the token issue transaction.
         */
        ACTION issue(name to, asset quantity, string memo);

        /**
         * @brief Retire action.
         * @details The opposite for create action, if all validations succeed,
         * it debits the statstable.supply amount.
         * @param quantity - the quantity of tokens to retire,
         * @param memo - the memo string to accompany the transaction.
         */
        ACTION retire(asset quantity, string memo);

        /**
         * @brief Standard transfer action.
         * @details Allows `from` account to transfer to `to` account the `quantity` tokens.
         * One account is debited and the other is credited with quantity tokens.
         * @param from - the account to transfer from,
         * @param to - the account to be transferred to,
         * @param quantity - the quantity of tokens to be transferred,
         * @param memo - the memo string to accompany the transaction.
         */
        ACTION transfer(name from, name to, asset quantity, string memo);
        
        /**
         * @brief used for cross chain transfers
         * @param from - sender of the amount should match target
         * @param to - receiver of the amount
         * @param amount_account - scope (target) of the transfer
         * @param amount_id - id of the intended transfer, containing amount
         * @param memo - memo for the transfer
         */
        ACTION transferbyid(name from, name to, name amount_account, uint64_t amount_id, string memo);

        /**
         * @brief Open action.
         * @details Allows `ram_payer` to create an account `owner` with zero balance for
         * token `symbol` at the expense of `ram_payer`.
         * @param owner - the account to be created,
         * @param symbol - the token to be payed with by `ram_payer`,
         * @param ram_payer - the account that supports the cost of this action.
         */
        ACTION open(name owner, symbol_code symbol, name ram_payer);
        
        /**
         * @brief Close action.
         * @details This action is the opposite for open, it closes the account `owner`
         * for token `symbol`.
         * @param owner - the owner account to execute the close action for,
         * @param symbol - the symbol of the token to execute the close action for.
         * @pre The pair of owner plus symbol has to exist otherwise no action is executed,
         * @pre If the pair of owner plus symbol exists, the balance has to be zero.
         */
        ACTION close(name owner, symbol_code symbol);

        /**
         * @brief Get supply method.
         * @details Gets the supply for token `sym_code`, created by `token_contract_account` account.
         * @param token_contract_account - the account to get the supply for,
         * @param sym - the symbol to get the supply for.
         */
        static asset get_supply(name token_contract_account, symbol_code sym) {
            stats statstable(token_contract_account, sym.raw());
            const auto& st = statstable.get(sym.raw());
            return st.supply;
        }

        /**
         * @brief Get balance method.
         * @details Get the balance for a token `sym_code` created by `token_contract_account` account,
         * for account `owner`.
         * @param token_contract_account - the token creator account,
         * @param owner - the account for which the token balance is returned,
         * @param sym - the token for which the balance is returned.
         */
        static asset get_balance(name token_contract_account, name owner, symbol_code sym) {
            accounts accountstable(token_contract_account, owner.value);
            const auto& ac = accountstable.get(sym.raw());
            return ac.balance;
        }

    private:
        void sub_balance(name owner, asset value);
        void add_balance(name owner, asset value, name ram_payer);

}; /** @}*/
