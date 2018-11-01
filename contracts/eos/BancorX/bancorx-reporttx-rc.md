## Action: reporttx Terms & Conditions

reports an incoming transaction from a different blockchain
can only be called by an existing reporter

reporter - reporter account
blockchain - name of the source blockchain
tx_id - unique transaction id on the source blockchain
target - target account on EOS
quantity -  asset and amount to issue to the target account if the minimum required number of reports is met
memo - memo to pass in in the transfer action
data - custom source blockchain value, usually a string representing the tx hash on the source blockchain
Contract
Issue a report by {{reporter}} that on the blockchain {{blockchain}} the transaction {{tx_id}} was registered to transfer {{quantity}} BNT to the EOS account {{target}} with the memo “{{memo}}” and the additional data “{{data}}”

If the minimum reporter's threshold for this transaction has been reached due to this report, execute the issuance of {{quantity}} BNT to {{target}}.

General clause. (1) this contract was designed and intended to be executed as an integral part of the Bancor Network and BancorX contractual frames (including as a specific action in a set of actions); (2) the Bancor Network contractual frame (which this contract relates to) was designed and is intended to execute transactions on different converters as designated in the conversion path; (3) the BancorX contractual frame (which this contract relates to) was designed and is intended to execute transactions under the parameters set under correlating actions (4) any use of this contract which deviates from its intended design and use, including any modifications, may not be supported by the Bancor Network and BancorX, nor render a compatible result.
