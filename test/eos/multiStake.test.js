

const chai = require('chai')
var assert = chai.assert

const {  
    snooze,
    expectError,
    expectNoError,
    randomAccount,
    expectNoErrorPrint
} = require('./common/utils')

const {  
    vote, stake, unstake, setConfig,
    getUserStake, getDelegates, 
    getWeights, getRefunds, getVotes
} = require('./common/multistake')

const { ERRORS } = require('./common/errors')
const { getConverter} = require('./common/converter')
const { getBalance, transfer } = require('./common/token')

const multiStaking = 'votingforugt'
const multiToken = 'multi4tokens'
const bntRelaySymbol = 'BNTEOS'
const bntRelay = 'bnt2eosrelay'
const user1 = 'bnttestuser1'
const user2 = 'bnttestuser2'

describe("Test: MultiStaking", () => {
    
    const deltaStr = '0.10000000'
    const amountToStakeStr = '2.00000000'
    const tooMuchStakeStr = '20000000.00000000'
    
    describe('un/staking and voting with un/delegation', async function() {
        this.timeout(4200)
        it("expect failures before setting config, and depositing properly", async () => { 
            await expectError(
                vote(),
                ERRORS.NO_CONFIG 
            )
            await expectNoError(
                setConfig()
            )
            await expectError(
                stake(deltaStr, bntRelaySymbol, user2, user2, true),
                ERRORS.ONLY_DEPOSIT
            )
            await expectError(
                stake(deltaStr, bntRelaySymbol, user2, user2, false),
                ERRORS.DEPOSIT_FIRST
            )
            await expectError(
                transfer(bntRelay, `${deltaStr} ${bntRelaySymbol}`, multiStaking, user2, ''),
                ERRORS.BAD_RELAY
            )
        })
        it("user2 stake to self without voting, user2 can't vote without staking", async () => {  
            // user2's balance before stake
            var result = await getBalance(user=user2, token=multiToken, symbol=bntRelaySymbol)
            const userBalanceBefore = parseFloat(result.rows[0].balance.split(' ')[0]).toFixed(8)
            
            // first successful stake before vote, user2 delegated to self
            await expectNoError( // user2's effective stake is 2.00000000
                transfer(multiToken, `${amountToStakeStr} ${bntRelaySymbol}`, multiStaking, user2, 'stake')
            ) 
            //TODO: make sure the events are firing

            result = await getUserStake(user2) 
            assert.equal(parseFloat(result.rows[0].staked.split(' ')[0])*100000000, result.rows[0].delegated_in, "delegatedin != staked")
            assert.equal(result.rows[0].staked, amountToStakeStr + " BNTEOS", "wrong value for staked")

            // staking contract's balance should have increased
            result = await getBalance(user=multiStaking, token=multiToken, symbol=bntRelaySymbol)
            const totalBalanceAfter = parseFloat(result.rows[0].balance.split(' ')[0])
            assert.equal(
                totalBalanceAfter, parseFloat(amountToStakeStr), 
                "staking contract's balance didn't properly increase"
            )
            //user 2's balance should have decreased
            result = await getBalance(user=user2, token=multiToken, symbol=bntRelaySymbol)
            const userBalanceAfter = parseFloat(result.rows[0].balance.split(' ')[0]).toFixed(8)
            assert.equal(
                parseFloat(userBalanceBefore - userBalanceAfter).toFixed(8), parseFloat(amountToStakeStr).toFixed(8), 
                "user1's balance didn't properly decrease"
            )
            await expectError(
                stake(tooMuchStakeStr, bntRelaySymbol, user2, user1, false), "cannot delegate out more than staked")
            
            // check that user2's row was not added to delegates table
            result = await getDelegates(user2, user2)
            assert.equal(result.rows.length, 0)
            
            // cannot vote before stake by user1, no effect on median yet 
            await expectError(vote('2.0000', user1)) 
        })
        it("user2 vote, then delegate majority of weight to user1, user1's vote 2% should be final median", async () => { 
            //median before stake, reading from settings
            result = await getConverter(bntRelaySymbol)
            assert.equal(result.rows.length, 1)   
            const medianBefore = result.rows[0].fee

            // first vote after stake, by user2 or anyone, shouldn't change median yet
            await expectNoError(vote('1.4242', user2))

            result = await getConverter(bntRelaySymbol)
            var newMedian = result.rows[0].fee
            assert.equal(medianBefore, newMedian, "median wasnt supposed to change")

            // only creates the median row in votes table and begins start_vote_after timer
            await snooze(2000) // must elapse start_vote_after
            
            // should change median
            await expectNoError(vote('1.0000', user2)) // user2's vote is 1%
            
            // first time median changed
            result = await getConverter(bntRelaySymbol)
            assert.equal(result.rows.length, 1)   
            newMedian = result.rows[0].fee
            assert.equal(newMedian, 10000, "median was supposed to change to 1%") 
        
            // first delegated stake to another user
            await expectNoError(stake("1.50000000", bntRelaySymbol, user2, user1, false))
            
            await expectError(vote('2.0000', user1)) // user1's vote is 2%, should change median
            
            result = await getUserStake(user2) 
            assert.equal(result.rows[0].delegated_out, 150000000, "wrong value for delegated out")
            assert.equal(result.rows[0].delegated_in, 50000000, "wrong value for delegated in")
                        
            result = await getUserStake(user1) 
            assert.equal(result.rows[0].delegated_to, 150000000, "wrong value for delegated to")

            result = await getDelegates(user2, user1)
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
            await expectError(stake("1.50000000", bntRelaySymbol, user1, user2, false))
        })
        it("user2 stake again, median should change back to 1%", async () => { 
            await expectNoError( // user2's effective stake is 2.00000000
                transfer(multiToken, `${amountToStakeStr} ${bntRelaySymbol}`, multiStaking, user2, '')
            ) 
            result = await getUserStake(user2) 
            const userStakedAfter = result.rows[0].staked
            const userDelegatedAfter = result.rows[0].delegated_in

            assert.equal(userStakedAfter, "4.00000000 BNTEOS", "user2's staked incorrect after stake")
            assert.equal(userDelegatedAfter, 250000000, "user2's delegated_in incorrect after stake")
            result = await getWeights()
            assert.equal(result.rows[0].total, 400000000, "total was supposed to change immediately after stake")
            assert.equal(result.rows[0].W[0], 250000000, "user2's weight accounted properly in weights after stake")

            result = await getConverter(bntRelaySymbol)
            assert.equal(result.rows.length, 1)   
            newMedian = result.rows[0].fee
            assert.equal(newMedian, 10000, "median was supposed to change back to 1%") 
        })
        it("user2 unstake", async () => { 
            // get totalStaked BEFORE unstake transaction
            var result = await getBalance(user=multiStaking, token=multiToken, symbol=bntRelaySymbol)
            const totalBalanceBefore = parseFloat(result.rows[0].balance.split(' ')[0]).toFixed(8)
            
            // get user's liquid balance BEFORE unstake transaction
            result = await getBalance(user=user2, token=multiToken, symbol=bntRelaySymbol)
            const userBalanceBefore = parseFloat(result.rows[0].balance.split(' ')[0]).toFixed(8)

            // perform actual unstake transaction
            await expectNoError(unstake(deltaStr, bntRelaySymbol, user2, user2, true))
    
            // make sure we got a row in the refunds table 
            result = await getRefunds()
            assert.equal(result.rows.length, 1, "refund didn't get created after unstake")
            
            // get total staked AFTER unstake transaction
            result = await getUserStake(user2) 
            const userStakedAfter = result.rows[0].staked
            const userDelegatedAfter = result.rows[0].delegated_in

            assert.equal(userStakedAfter, "3.90000000 BNTEOS", "user2's staked incorrect after unstake")
            assert.equal(userDelegatedAfter, 240000000, "user2's delegated_in incorrect after unstake")
            
            result = await getWeights()
            assert.equal(result.rows[0].total, 390000000, "total was supposed to change immediately after unstake")
            assert.equal(result.rows[0].W[0], 240000000, "user2's weight accounted properly in weights after unstake")

            result = await getConverter(bntRelaySymbol)
            assert.equal(result.rows.length, 1)   
            newMedian = result.rows[0].fee
            assert.equal(newMedian, 10000, "median wasn't supposed to change") 

            //
            await snooze(1000)
            
            // make sure refund was fulfilled and row was cleared
            result = await getRefunds()
            if (result.rows.length != 0) {
                result = await refund()
                result = await getRefunds()
            }
            assert.equal(result.rows.length, 0, "refund didn't get cleared")
    
            // get contract's liquid balance AFTER unstake transaction
            result = await getBalance(user=multiStaking, token=multiToken, symbol=bntRelaySymbol)
            const totalBalanceAfter = parseFloat(result.rows[0].balance.split(' ')[0]).toFixed(8)
            assert.equal(parseFloat(totalBalanceBefore - totalBalanceAfter).toFixed(8), parseFloat(deltaStr).toFixed(8), "total staked didn't decrease properly after unstake")   
            
            // get user2 liquid balance AFTER
            result = await getBalance(user=user2, token=multiToken, symbol=bntRelaySymbol)
            const userBalanceAfter = parseFloat(result.rows[0].balance.split(' ')[0]).toFixed(8)
            assert.equal(parseFloat(userBalanceAfter - userBalanceBefore).toFixed(8), parseFloat(deltaStr).toFixed(8), "liquid balance didn't increase properly after refund")   
        })
        it("user2 undelegate a bit, check weight change successful", async () => { 
        
        })   
        it("user2 undelegate all the way to zero with transfer, user1 should be gone from weights", async () => { 
        
        })   
        it("user2 change vote, weights should change (old vote gone)", async () => { 
        
        })   
        it("user2 delegate with transfer to user1", async () => { 
        
        })   
        it("user2 unstake all the way to zero, median and weights should change to user1's", async () => { 
        
        })  
        it("user1 undelegate from self -- should throw ", async () => { 
            
        }) 
        it("user1 unstake all the way to zero, median shouldn't change if total stake is 0", async () => { 
        
        })  
        it("rick refund yudi NON exist -- should throw", async() => {
            
        })
        it("yudi refund rick TOO soon -- should throw", async() => {
            
        })
        it("median should NOT be updated in converter yet", async () => { 
    
        }) 
    })
    describe('voting algorithm edge cases', async function() {
        it("make big delta, record result, revert it, make many small ones to equal big one", async () => { 
        })   
        it("vote more stake for 0, for all 29998 possible fees after with same stake, 30000 with largest stake", async () => { /*
            //VOTE 29 000 TIMES, default amt,
            //record time
            //place stake and fee to 30000
            //record time difference

            const acct = await expectNoError(randomAccount())
            console.log(acct)
            //transfer 0.00000001 BNTEOS to account
            //stake BNTEOS
            //vote
        */})
    })
})
