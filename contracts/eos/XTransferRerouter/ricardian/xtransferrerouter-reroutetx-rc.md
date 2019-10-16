## Action: reroutetx(uint64_t tx_id, string blockchain, string target) Terms & Conditions

Following an existing BNT token xtransfer action that failed, Reports an updated destination for the transaction

tx_id - unique transaction id on the source blockchain
blockchain - name of the destination blockchain
target - correct target account on the destination Blockchain (meaning the target account to which the BNT tokens will transfer to)

Contract
The {{reporter}}, amends and reroutes an existing transaction {{tx_id}} which is not fulfilled and has expired (due to errors inserted by {{reporter}} in the original transaction), as follows: the original transaction {{tx_id}} will be executed with destination blockchain being {{blockchain}}, and destination address being {{target}}.

Other than the above, no other changes will be made or apply to the original transaction {{tx_id}} which shall remain unaffected including the amount


General clause. (1) this contract was designed and intended to be executed as an integral part of the Bancor Network and BancorX contractual frames (including as a specific action in a set of actions); (2) the Bancor Network contractual frame (which this contract relates to) was designed and is intended to execute transactions on different converters as designated in the conversion path; (3) the BancorX contractual frame (which this contract relates to) was designed and is intended to execute transactions under the parameters set under correlating actions (4) any use of this contract which deviates from its intended design and use, including any modifications, may not be supported by the Bancor Network and BancorX, nor render a compatible result.