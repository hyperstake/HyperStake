#include <map>
#include <string>
#include <boost/test/unit_test.hpp>
#include "json/json_spirit_writer_template.h"

#include "main.h"
#include "wallet.h"
#include "../voteproposal.h"
#include "../voteobject.h"
#include "../votetally.h"
#include "../votecharset.h"
#include "../voteproposalmanager.h"
#include "../db.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(voting_tests)

// name of issue
std::string strName = "proposal1";
// check version for existing proposals Shift
uint8_t nShift = 20;
// start time - will be changed to int StartHeight. unix time stamp
int64 nStartTime =  10000000;
// number of blocks with votes to count
int nCheckSpan = 1000;
// cardinal items to vote on - convert to uint8 CheckSpan
uint8_t nCardinals = 2;
// description of issue - will go in different tx
std::string strDescription = "test_description";

BOOST_AUTO_TEST_CASE(proposal_serialization)
{
    std::cout << "testing proposal serialization\n";

    CVoteProposalManager manager;

    VoteLocation location;
    manager.GetNextLocation(nCardinals, nStartTime, nCheckSpan, location);
    CVoteProposal proposal(strName, nStartTime, nCheckSpan, strDescription, location);
    manager.Add(proposal);

    //! Add the constructed proposal to a partial transaction
    CTransaction tx;
    BOOST_CHECK_MESSAGE(proposal.ConstructTransaction(tx), "failed to construct tx");

    BOOST_CHECK_MESSAGE(tx.IsProposal(), "Transaction is not a proposal!");

    CVoteProposal proposal2;
    BOOST_CHECK_MESSAGE(ProposalFromTransaction(tx, proposal2), "Failed to deserialize");

    //CHECKS TO DETERMINE IF DESERIALIZATION WORKS AS INTENDED
    BOOST_CHECK_MESSAGE(proposal2.GetHash() == proposal.GetHash(), "hash of proposal after it is deserialized isn't equal"
                                                                   "to its original value");
    BOOST_CHECK_MESSAGE(proposal2.GetLocation().nLeastSignificantBit == proposal.GetLocation().nLeastSignificantBit
                            && proposal2.GetLocation().nMostSignificantBit == proposal.GetLocation().nMostSignificantBit,
                            "location of proposal after it is deserialized isn't equal to its original value");
    BOOST_CHECK_MESSAGE(proposal2.GetBitCount() == proposal.GetBitCount(), "bitcount of proposal after it is deserialized isn't equal"
                                                                   "to its original value");
    BOOST_CHECK_MESSAGE(proposal2.GetShift() == proposal.GetShift(), "shift of proposal after it is deserialized isn't equal"
                                                                   "to its original value");
    BOOST_CHECK_MESSAGE(proposal2.GetStartHeight() == proposal.GetStartHeight(), "start height of proposal after it is deserialized isn't equal"
                                                                                 "to its original value");
    BOOST_CHECK_MESSAGE(proposal2.GetCheckSpan() == proposal.GetCheckSpan(), "check span of proposal after it is deserialized isn't equal"
                                                                   "to its original value");
    BOOST_CHECK_MESSAGE(proposal2.GetDescription() == proposal.GetDescription(), "description of proposal after it is deserialized isn't equal"
                                                                   "to its original value");
    BOOST_CHECK_MESSAGE(proposal2.GetName() == proposal.GetName(), "name of proposal after it is deserialized isn't equal"
                                                                   "to its original value");

    //! Create a tx that can be used as an input in the actual proposal tx
    CTransaction txFunding;
    CTxOut outTemp(10 * COIN, CScript());
    txFunding.vout.push_back(outTemp);
    mempool.addUnchecked(txFunding.GetHash(), txFunding);

    //! Add an input to the proposal tx
    tx.vin.push_back(CTxIn(txFunding.GetHash(), 0));
    tx.vout[0].nValue = MIN_TXOUT_AMOUNT;
    cout << tx.ToString() << "\n";

    //! Return anything beyond the proposal fee (5 HYP) as change UTXO
    int64 nFee = 5 * COIN;
    nFee -= MIN_TXOUT_AMOUNT; //Had to assign mintxamt to the utxo containing the proposal
    int64 nChange = outTemp.nValue - nFee;
    if (nChange > 500) {
        CKey key;
        key.MakeNewKey(true);

        CScript scriptReturn;
        scriptReturn << OP_DUP << OP_HASH160 << ToByteVector(Hash160(scriptReturn)) << OP_EQUALVERIFY << OP_CHECKSIG;
        CTxOut out(nChange, scriptReturn);

        //Add the change output to the new transaction
        tx.vout.push_back(out);
    }

    BOOST_CHECK_MESSAGE(tx.CheckTransaction(), "check transaction failed");

    CTxDB txdb("r");
    BOOST_CHECK_MESSAGE(tx.AcceptToMemoryPool(txdb, false), "transaction not accepted to mempool");

}

//BOOST_AUTO_TEST_CASE(vote_tally)
//{
    /*std::cout << "testing vote tally\n";

    map<uint256, VoteLocation> mapNewLocations;
    VoteLocation location(proposal.GetShift() + proposal.GetBitCount() - 1, proposal.GetShift());
    mapNewLocations.insert(make_pair(proposal.GetHash(), location));

    CTransaction tx;
    proposal.ConstructTransaction(tx);

    mapProposals.insert(std::pair<uint256, uint256>(tx.GetHash(), proposal.GetHash()));

    CVoteTally tally;
    BOOST_CHECK_MESSAGE(tally.SetNewPositions(mapNewLocations), "Position is already occupied when it should not be");

    uint32_t nVote = 0x40080000;
    // 0100 0000 0000 1000 0000 0000 0000 0000
    //
    tally.ProcessNewVotes(nVote);

    CVoteSummary summary;
    BOOST_CHECK_MESSAGE(tally.GetSummary(proposal.GetHash(), summary), "failed to get summary from tally");
    BOOST_CHECK_MESSAGE(summary.nYesTally == 1, "summary is not 1");
    std::cout << summary.nYesTally << endl;

    //Start a new tally object for the next block
    CVoteTally tally2(tally);
    tally2.ProcessNewVotes(nVote);
    CVoteSummary summary2;
    BOOST_CHECK_MESSAGE(tally2.GetSummary(proposal.GetHash(), summary2), "failed to get summary from tally2");
    BOOST_CHECK_MESSAGE(summary2.nYesTally == 2, "summary2 is not 2");

    //Start a new tally object for the next block - vote no
    CVoteTally tally3(tally2);
    nVote = 0x40180011;
    tally3.ProcessNewVotes(nVote);
    CVoteSummary summary3;
    BOOST_CHECK_MESSAGE(tally3.GetSummary(proposal.GetHash(), summary3), "failed to get summary from tally2");
    BOOST_CHECK_MESSAGE(summary3.nYesTally == 2, "summary3 is not 2");
    BOOST_CHECK_MESSAGE(summary3.nNoTally == 1, "summary3 no votes is not 1");

    //Start a new tally object for the next block - vote abstain
    CVoteTally tally4(tally3);
    nVote = 0x40000011;
    tally4.ProcessNewVotes(nVote);
    CVoteSummary summary4;
    BOOST_CHECK_MESSAGE(tally4.GetSummary(proposal.GetHash(), summary4), "failed to get summary from tally2");
    BOOST_CHECK_MESSAGE(summary4.nYesTally == 2, "summary4 is not 2");
    BOOST_CHECK_MESSAGE(summary4.nNoTally == 1, "summary4 no votes is not 1");*/

//}

BOOST_AUTO_TEST_CASE(vote_charset)
{
        std::cout << "testing proposal manager functionality and GetCombinedVotes()" << endl;

        CVoteProposalManager manager;

        VoteLocation location1;
        manager.GetNextLocation(nCardinals, nStartTime, nCheckSpan, location1);
        CVoteProposal proposal1(strName, nStartTime, nCheckSpan, strDescription, location1);
        manager.Add(proposal1);

        VoteLocation location2;
        manager.GetNextLocation(nCardinals, nStartTime, nCheckSpan, location2);
        CVoteProposal proposal2(strName, nStartTime, nCheckSpan, strDescription, location2);
        manager.Add(proposal2);

        VoteLocation location3;
        manager.GetNextLocation(nCardinals, nStartTime, nCheckSpan, location3);
        CVoteProposal proposal3(strName, nStartTime, nCheckSpan, strDescription, location3);
        manager.Add(proposal3);

        VoteLocation location4;
        manager.GetNextLocation(nCardinals, nStartTime, nCheckSpan, location4);
        CVoteProposal proposal4(strName, nStartTime, nCheckSpan, strDescription, location4);
        manager.Add(proposal4);

        VoteLocation location5;
        manager.GetNextLocation(nCardinals, nStartTime, nCheckSpan, location5);
        CVoteProposal proposal5(strName, nStartTime, nCheckSpan, strDescription, location5);
        manager.Add(proposal5);

        uint32_t nVersion = 0x50000000;

        uint32_t nVote1 = 2;
        CVoteObject vote1(proposal1.GetHash(), proposal1.GetLocation());
        vote1.Vote(nVote1);

        uint32_t nVote2 = 1;
        CVoteObject vote2(proposal2.GetHash(), proposal2.GetLocation());
        vote2.Vote(nVote2);

        uint32_t nVote3 = 2;
        CVoteObject vote3(proposal3.GetHash(), proposal3.GetLocation());
        vote3.Vote(nVote3);

        uint32_t nVote4 = 1;
        CVoteObject vote4(proposal4.GetHash(), proposal4.GetLocation());
        vote4.Vote(nVote4);

        uint32_t nVote5 = 2;
        CVoteObject vote5(proposal5.GetHash(), proposal5.GetLocation());
        vote5.Vote(nVote5);

        std::vector<CVoteObject> votes = {vote1, vote2, vote3, vote4, vote5};

        nVersion |= CVoteObject::GetCombinedVotes(votes);

        // 0101 [10 01] [10 01] [10 00] 0000 0000 0000 0000
        uint32_t correctResult = 0x59980000;

        std::cout << "Expected result: " << correctResult << endl << "Actual result: " << nVersion << endl;

        BOOST_CHECK_MESSAGE(nVersion == correctResult, "votes were not combined correctly");
};

BOOST_AUTO_TEST_SUITE_END()
