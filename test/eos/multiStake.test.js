

const chai = require('chai')
var assert = chai.assert

const {  
    snooze,
    expectError,
    expectNoError,
    randomAccount,
} = require('./common/utils')

const {  
    vote, undelegate, delegate, setConfig,
    getUserStake, getDelegate, getWeights,
    getRefunds, getVotes, refund
} = require('./common/multistake')

const { ERRORS } = require('./common/errors')
const { getBalance, transfer } = require('./common/token')
const { getConverter, setMaxfee } = require('./common/converter')

describe("Test: MultiStaking", () => {
    const multiConverter = 'multiconvert'
    const multiStaking = 'multistaking'
    const multiToken = 'multi4tokens'
    const bntRelaySymbol = 'BNTEOS'
    const bntRelay = 'bnt2eosrelay'
    const user1 = 'bnttestuser1'
    const user2 = 'bnttestuser2'

    const deltaStr = '0.10000000'
    const amountToStakeStr = '2.00000000'
    const tooMuchStakeStr = '20000000.00000000'
    
    describe('un/staking and voting with un/delegation', async function() {
        this.timeout(4242)
        it("expect failures before setting config, and depositing properly", async () => { 
            await expectError(
                vote(),
                ERRORS.NO_CONFIG 
            )
            await expectNoError(
                setMaxfee(multiConverter, 42000)
            )
            await expectNoError(
                setConfig()
            )
            await expectError(
                delegate(deltaStr, bntRelaySymbol, user2, user2, true),
                ERRORS.ONLY_DEPOSIT
            )
            await expectError(
                delegate(deltaStr, bntRelaySymbol, user2, user2, false),
                ERRORS.DEPOSIT_FIRST
            )
            await expectError(
                transfer(bntRelay, `${deltaStr} ${bntRelaySymbol}`, multiStaking, user2, 'stake'),
                ERRORS.BAD_RELAY
            )
        })
        it("user1 stake to self without voting, user2 can't vote without staking", async () => {  
            // user1's balance before stake
            var result = await getBalance(user=user1, token=multiToken, symbol=bntRelaySymbol)
            const userBalanceBefore = parseFloat(result.rows[0].balance.split(' ')[0]).toFixed(8)
            
            // first successful stake before vote, user1 staking to self
            const res = await expectNoError( // user1's effective stake is 2.00000000
                transfer(multiToken, `${amountToStakeStr} ${bntRelaySymbol}`, multiStaking, user1, 'stake')
            ) 
            const events = JSON.parse(res.processed.action_traces[0].inline_traces[1].console.split("\n")[0])

            result = await getUserStake(user1) 
            assert.equal(parseFloat(result.rows[0].staked.split(' ')[0])*100000000, result.rows[0].delegated_in, "delegated_in != staked")
            assert.equal(result.rows[0].staked, amountToStakeStr + " BNTEOS", "wrong value for staked")
            assert.equal(events.amount, amountToStakeStr + " BNTEOS", "wrong value for event - amount")

            // staking contract's balance should have increased
            result = await getBalance(user=multiStaking, token=multiToken, symbol=bntRelaySymbol)
            const totalBalanceAfter = parseFloat(result.rows[0].balance.split(' ')[0])
            assert.equal(
                totalBalanceAfter, parseFloat(amountToStakeStr), 
                "staking contract's balance didn't properly increase"
            )
            //user 1's balance should have decreased
            result = await getBalance(user=user1, token=multiToken, symbol=bntRelaySymbol)
            const userBalanceAfter = parseFloat(result.rows[0].balance.split(' ')[0]).toFixed(8)
            assert.equal(
                parseFloat(userBalanceBefore - userBalanceAfter).toFixed(8), parseFloat(amountToStakeStr).toFixed(8), 
                "user1's balance didn't properly decrease"
            )
            await expectError(
                delegate(tooMuchStakeStr, bntRelaySymbol, user1, user2, false), 
                "cannot delegate out more than staked"
            )
            
            // check that user1's row was not added to delegates table
            result = await getDelegate(user1, user1)
            assert.equal(result.rows.length, 0)
            
            // cannot vote before stake by user2, no effect on median yet 
            await expectError(
                vote('2.0000', user2), 
                "cannot vote if user has no weight"
            ) 
        })
        it("user1 vote, then delegate majority of weight to user2, user2's vote 2% should be final median", async () => { 
            //median before stake, reading from settings
            result = await getConverter(bntRelaySymbol)
            assert.equal(result.rows.length, 1)   
            const medianBefore = result.rows[0].fee

            // first vote after stake, by user1 or anyone, shouldn't change median yet
            await expectNoError(
                vote('1.4242', user1)
            )

            result = await getConverter(bntRelaySymbol)
            var newMedian = result.rows[0].fee
            assert.equal(medianBefore, newMedian, "median wasnt supposed to change")

            // only creates the median row in votes table and begins start_vote_after timer
            await snooze(1000) // must elapse start_vote_after
            
            // should change median
            await expectNoError(
                vote('1.0000', user1)
            ) // user1's vote is 1%
            
            // first time median changed
            result = await getConverter(bntRelaySymbol)
            assert.equal(result.rows.length, 1)   
            newMedian = result.rows[0].fee
            assert.equal(newMedian, 10000, "median was supposed to change to 1%") 
        
            // first delegated stake to another user
            await expectNoError(
                delegate("1.50000000", bntRelaySymbol, user1, user2, false)
            )
            
            await expectNoError(
                vote('2.0000', user2)
            ) // user1's vote is 2%, should change median
            
            result = await getUserStake(user1) 
            assert.equal(result.rows[0].delegated_out, 150000000, "wrong value for delegated out")
            assert.equal(result.rows[0].delegated_in, 50000000, "wrong value for delegated in")
                        
            result = await getUserStake(user2) 
            assert.equal(result.rows[0].delegated_to, 150000000, "wrong value for delegated to")

            result = await getDelegate(user1, user2)
            assert.equal(result.rows.length, 1)

            result = await getWeights()
            assert.equal(result.rows[0].total, 200000000, "total not supposed to change")
            assert.equal(result.rows[0].Y[1], 20000, "user1's fee wasn't accounted in weights")
            assert.equal(result.rows[0].W[0], 50000000, "user1's fee wasn't accounted in weights")
            assert.equal(result.rows[0].W[1], 150000000, "user1's fee wasn't accounted in weights")

            // second time median changed
            result = await getConverter(bntRelaySymbol)
            assert.equal(result.rows.length, 1)   
            newMedian = result.rows[0].fee
            assert.equal(newMedian, 20000, "median was supposed to change to 2%") 

            // can't delegate more than staked
            await expectError(
                delegate("1.50000000", bntRelaySymbol, user2, user1, false), 
                "cannot delegate out more than staked"
            )
        })
        it("user1 stake again, median should change back to 1%", async () => { 
            await expectNoError( // user1's effective stake is 2.00000000
                transfer(multiToken, `${amountToStakeStr} ${bntRelaySymbol}`, multiStaking, user1, 'stake')
            ) 
            result = await getUserStake(user1) 
            const userStakedAfter = result.rows[0].staked
            const userDelegatedAfter = result.rows[0].delegated_in

            assert.equal(userStakedAfter, "4.00000000 BNTEOS", "user1's staked incorrect after stake")
            assert.equal(userDelegatedAfter, 250000000, "user1's delegated_in incorrect after stake")
            result = await getWeights()
            assert.equal(result.rows[0].total, 400000000, "total was supposed to change immediately after stake")
            assert.equal(result.rows[0].W[0], 250000000, "user1's weight accounted properly in weights after stake")

            result = await getConverter(bntRelaySymbol)
            assert.equal(result.rows.length, 1)   
            newMedian = result.rows[0].fee
            assert.equal(newMedian, 10000, "median was supposed to change back to 1%") 
        })
        it("user1 unstake", async () => { 
            // get totalStaked BEFORE unstake transaction
            var result = await getBalance(user=multiStaking, token=multiToken, symbol=bntRelaySymbol)
            const totalBalanceBefore = parseFloat(result.rows[0].balance.split(' ')[0]).toFixed(8)
            
            // get user's liquid balance BEFORE unstake transaction
            result = await getBalance(user=user1, token=multiToken, symbol=bntRelaySymbol)
            const userBalanceBefore = parseFloat(result.rows[0].balance.split(' ')[0]).toFixed(8)

            // perform actual unstake transaction
            await expectNoError(
                undelegate(deltaStr, bntRelaySymbol, user1, user1, true)
            )
            await expectError( //user1's refund cannot be deployed prematurely
                refund(), "refund is not available for retrieval yet" 
            )
            // make sure we got a row in the refunds table 
            result = await getRefunds()
            assert.equal(result.rows.length, 1, "refund didn't get created after unstake")
            
            // get total staked AFTER unstake transaction
            result = await getUserStake(user1) 
            const userStakedAfter = result.rows[0].staked
            const userDelegatedAfter = result.rows[0].delegated_in

            assert.equal(userStakedAfter, "3.90000000 BNTEOS", "user1's staked incorrect after unstake")
            assert.equal(userDelegatedAfter, 240000000, "user1's delegated_in incorrect after unstake")
            
            result = await getWeights()
            assert.equal(result.rows[0].total, 390000000, "total was supposed to change immediately after unstake")
            assert.equal(result.rows[0].W[0], 240000000, "user1's weight not accounted properly in weights after unstake")

            result = await getConverter(bntRelaySymbol)
            assert.equal(result.rows.length, 1)   
            newMedian = result.rows[0].fee
            assert.equal(newMedian, 10000, "median wasn't supposed to change") 
            
            //
            await snooze(1000)
            
            // make sure refund was fulfilled and row was cleared
            result = await getRefunds()
            assert.equal(result.rows.length, 0, "refund didn't get cleared")

            await expectError( //user1's refund cannot be deployed prematurely
                refund(), "refund request not found"
            )
    
            // get contract's totalStaked AFTER unstake transaction
            result = await getBalance(user=multiStaking, token=multiToken, symbol=bntRelaySymbol)
            const totalBalanceAfter = parseFloat(result.rows[0].balance.split(' ')[0]).toFixed(8)
            assert.equal(parseFloat(totalBalanceBefore - totalBalanceAfter).toFixed(8), parseFloat(deltaStr).toFixed(8), "total staked didn't decrease properly after unstake")   
            
            // get user1 liquid balance AFTER
            result = await getBalance(user=user1, token=multiToken, symbol=bntRelaySymbol)
            const userBalanceAfter = parseFloat(result.rows[0].balance.split(' ')[0]).toFixed(8)
            assert.equal(parseFloat(userBalanceAfter - userBalanceBefore).toFixed(8), parseFloat(deltaStr).toFixed(8), "liquid balance didn't increase properly after refund")   
        })
        it("user2 undelegate a bit, check weight changes successful", async () => { 
            var result = await getUserStake(user2) 
            const user2StakedBefore = result.rows[0].staked
            const user2DelegatedInBefore = result.rows[0].delegated_in
            const user2DelegatedOutBefore = result.rows[0].delegated_out

            result = await getWeights()
            const totalBefore = result.rows[0].total

            result = await getConverter(bntRelaySymbol)
            const oldMedian = result.rows[0].fee

            await expectNoError(
                undelegate(deltaStr, bntRelaySymbol, user2, user1, false)
            )

            result = await getUserStake(user2) 
            const user2StakedAfter = result.rows[0].staked
            const user2DelegatedInAfter = result.rows[0].delegated_in
            const user2DelegatedOutAfter = result.rows[0].delegated_out
            const user2DelegatedToAfter = result.rows[0].delegated_to

            result = await getUserStake(user1) 
            const user1StakedBefore = result.rows[0].staked
            var user1DelegatedIn = result.rows[0].delegated_in

            assert.equal(user2DelegatedInAfter, user2DelegatedInBefore, "user2's 'delegated_in' wasn't supposed to change")
            assert.equal(user2DelegatedOutAfter, user2DelegatedOutBefore, "user2's 'delegated_out' wasn't supposed to change")
            assert.equal(user2StakedAfter, user2StakedBefore, "user2's 'staked' wasn't supposed to change")
            assert.equal(user2DelegatedToAfter, 140000000, "user2's 'delegated_to' did not correctly decrease")
            assert.equal(user1DelegatedIn, 250000000, "user1's 'delegated_in' did not correctly increase")
            
            await expectError(
                undelegate(deltaStr, bntRelaySymbol, user1, user2, true),
                "non-existant delegation"   
            )

            result = await getWeights()
            var totalAfter = result.rows[0].total
            assert.equal(totalAfter, totalBefore, "total was not supposed to change")

            await expectNoError(
                undelegate(deltaStr, bntRelaySymbol, user2, user1, true)
            )

            result = await getUserStake(user1) 
            const user1StakedAfter = result.rows[0].staked
            
            const deltaStaked = parseFloat(user1StakedBefore.split(' ')[0]) - parseFloat(user1StakedAfter.split(' ')[0])
            assert.equal(deltaStaked.toFixed(8), deltaStr, "user1's staked did not correctly decrease") 

            result = await getWeights()
            totalAfter = result.rows[0].total
            const deltaTotal = parseInt(totalBefore) - parseInt(totalAfter)
            
            assert.equal(deltaTotal / 100000000, deltaStr, "total did not correctly decrease") 
            assert.equal(deltaStaked.toFixed(8), deltaStr, "user1's staked did not correctly decrease") 
            
            assert.equal(result.rows[0].W[0], user1DelegatedIn, "user1's weight was not supposed to change")
            assert.equal(result.rows[0].W[1], 130000000, "user2's weight not accounted properly in weights after unstake")

            result = await getConverter(bntRelaySymbol)
            newMedian = result.rows[0].fee
            assert.equal(newMedian, oldMedian, "median wasn't supposed to change")
        })
        it("user1 change vote, weights should change (old vote gone)", async () => { 
            // should change median
            await expectNoError(
                vote('2.5000', user1)
            ) 

            var result = await getConverter(bntRelaySymbol)
            assert.equal(result.rows.length, 1)   
            newMedian = result.rows[0].fee
            assert.equal(newMedian, 25000, "median was supposed to change to 2.5%")
            
            result = await getWeights()
            assert.equal(result.rows[0].W[0], 130000000, "user2's weight was supposed to become the first one")
            assert.equal(result.rows[0].W[1], 250000000, "user1's weight was supposed to become the second one")
        })
        it("user1 undelegate all the way to zero with transfer, user2 should be gone from weights", async () => { 
            await expectError(
                undelegate("1.50000000", bntRelaySymbol, user1, user2, true), 
                "non-existant delegation"
            )
            await expectError(
                undelegate("1.50000000", bntRelaySymbol, user2, user1, true, user2), 
                "missing authority of bnttestuser1"
            )
            await expectError(
                undelegate("1.50000000", bntRelaySymbol, user2, user1, true),
                "insufficient delegated stake"
            )
            await expectNoError(
                undelegate("1.30000000", bntRelaySymbol, user2, user1, true)
            )
            const result = await getUserStake(user2) 
            assert.equal(result.rows.length, 0, "user2 didn't get ramfree'd")
        })   
        it("user1 delegate with transfer to user1", async () => {             
            var result = await getUserStake(user1)
            const user1DelegatedOutBefore = result.rows[0].delegated_out
            const user2DelegatedToBefore = 0 

            // first delegated stake with transfer 
            await expectNoError(
               delegate("1.50000000", bntRelaySymbol, user1, user2, true)
            )

            result = await getUserStake(user1)
            const user1StakedAfter = result.rows[0].staked
            const user1DelegatedInAfter = result.rows[0].delegated_in
            const user1DelegatedOutAfter = result.rows[0].delegated_out
            
            result = await getUserStake(user2)
            const user2StakedAfter = result.rows[0].staked
            const user2DelegatedInAfter = result.rows[0].delegated_in
            const user2DelegatedToAfter = result.rows[0].delegated_to

            result = await getWeights()
            const totalAfter = result.rows[0].total
            
            assert.equal(totalAfter, 100000000, "total was supposed to decrease because user 1 needs to vote again")
            assert.equal(user1DelegatedOutBefore, user1DelegatedOutAfter, "user1del_out wasn't supposed to change")
            assert.equal(user2DelegatedToBefore, user2DelegatedToAfter, "user2del_to wasn't supposed to change")
            
            assert.equal(user1StakedAfter.split(' ')[0], "1.00000000", "user1staked didn't appropriately decrease")
            assert.equal(user1DelegatedInAfter, 100000000, "user1del_in didn't appropriately decrease")
            assert.equal(user2StakedAfter.split(' ')[0], "1.50000000", "user2staked didn't appropriately increase")
            assert.equal(user2DelegatedInAfter, 150000000, "user2del_in didn't appropriately increase")
        })   
        it("user1 unstake all the way to zero, median and weights should change to user2's", async () => {
            var result = await getConverter(bntRelaySymbol)
            oldMedian = result.rows[0].fee

            await expectNoError(
                undelegate("1.00000000", bntRelaySymbol, user1, user1, true)
            )
            await expectError(
                undelegate("0.00000001", bntRelaySymbol, user1, user1, true),
                "stake doesn't exist"
            )
            result = await getUserStake(user1) 
            assert.equal(result.rows.length, 0, "user1 didn't get ramfree'd") 
            
            result = await getConverter(bntRelaySymbol)
            newMedian = result.rows[0].fee

            assert.equal(newMedian, oldMedian, "median wasn't supposed to change")
        })
    })
})
