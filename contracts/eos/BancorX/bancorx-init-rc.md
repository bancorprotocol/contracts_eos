## Action: transfer Terms & Conditions

This action is intended to Initializes the contract settings. 
It can only be called once, by the contract account.

Parameters:

    1. X_token_name - cross-chain token account
    2. Min_reporters - the minimum required number of reporters to fulfill a cross-chain TX
    3. Min_limit - minimum amount that can be transferred out (destroyed)
    4. Limit_inc - the amount the current limit is increased by in each interval (block)
    5. Max_issue_limit - the maximum amount that can be issued on EOS in a given timespan
    6. Max_destroy_limit - the maximum amount that can be transferred out (destroyed) in a given timespan

Contract:

Initialize cross-chain transfer contract for the token {{X_token_name}} with the following settings:

A minimum of {{min_reporters}} different reporters are required in order to issue new tokens.

The minimum amount of tokens that can be transferred in a single outbound transaction is {{min_limit}}

The amounts of BNT that can be issued and destroyed in any specific timeframe is limited. Limits are applied on issuance and destruction of the token, and both are increased by {{limit_inc}} BNTs every block, up to a maximum of {{max_issue_limit}} BNT for the Issuance Limit, and a maximum of {{max_destroy_limit}} BNT for the destruction limit.

General clause. (1) this contract was designed and intended to be executed as an integral part of the Bancor Network and BancorX contractual frames (including as a specific action in a set of actions); (2) the Bancor Network contractual frame (which this contract relates to) was designed and is intended to execute transactions on different converters as designated in the conversion path; (3) the BancorX contractual frame (which this contract relates to) was designed and is intended to execute transactions under the parameters set under correlating actions (4) any use of this contract which deviates from its intended design and use, including any modifications, may not be supported by the Bancor Network and BancorX, nor render a compatible result.
