#include <map>
#include <string>
#include <boost/test/unit_test.hpp>
#include "../json/json_spirit_writer_template.h"

#include "../main.h"
#include "../wallet.h"
#include "../voteproposal.h"
#include "../voteobject.h"
#include "../votetally.h"
#include "../votecharset.h"
#include "../voteproposalmanager.h"
#include "../db.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(voting_tests)

std::string CONSTRUCT_ERROR = "failed to construct tx";
std::string NOT_PROPOSAL_ERROR = "Transaction is not a proposal";
std::string COULD_NOT_DESERIALIZE_ERROR = "Failed to deserialize";
std::string BIT_LOCATION_ERROR = "location of proposal after it is deserialized isn't equal to its original value";
std::string BIT_COUNT_ERROR = "bitcount of proposal after it is deserialized isn't equal to its original value";
std::string SHIFT_ERROR = "shift of proposal after it is deserialized isn't equal to its original value";
std::string START_HEIGHT_ERROR = "start height of proposal after it is deserialized isn't equal to its original value";
std::string SPAN_ERROR = "check span of proposal after it is deserialized isn't equal to its original value";
std::string DESCRIPTION_ERROR = "description of proposal after it is deserialized isn't equal to its original value";
std::string NAME_ERROR = "name of proposal after it is deserialized isn't equal to its original value";
std::string HASH_ERROR = "hash of proposal after it is deserialized isn't equal to its original value";
std::string REFUND_ADDRESS_ERROR = "refund address is not equal to its original value after deserialization";
std::string MAX_FEE_ERROR = "the max fee of the proposal after it is deserialized isn't equal to its original value";
std::string DIFFERENT_ORDER_ERROR = "the deterministic ordering should be the same for both vectors but isn't";
std::string SAME_ORDER_ERROR = "the deterministic orderings of the same vector for two different proofhashes are the same";
std::string CREATE_ORDER_ERROR = "failed to construct deterministic ordering of proposals";
std::string REFUND_OUT_ERROR = "the refund outputs in the coinbase tx were not constructed correctly";

// name of issue
std::string strName = "ppppppppp";
// check version for existing proposals Shift
uint8_t nShift = 20;
// start time - will be changed to int StartHeight. unix time stamp
int64 nStartHeight =  1;
// number of blocks with votes to count
int nCheckSpan = 1000;
// cardinal items to vote on - convert to uint8 CheckSpan
uint8_t nCardinals = 2;
// description of issue - will go in different tx
std::string strDescription = "ppppppppppppppppppppppppppp";

BOOST_AUTO_TEST_CASE(proposal_serialization)
{
    std::cout << "testing proposal serialization\n";

    CVoteProposalManager manager;
    VoteLocation location;
    manager.GetNextLocation(nCardinals, nStartHeight, nCheckSpan, location);
    CVoteProposal proposal(strName, nStartHeight, nCheckSpan, strDescription, 5000000, "kxpSiwFkHJdNnYFojNDAvP3iiYyw3GH6ZL");
    manager.Add(proposal);

    //! Add the constructed proposal to a partial transaction
    CTransaction tx;
    BOOST_CHECK_MESSAGE(proposal.ConstructTransaction(tx), CONSTRUCT_ERROR);

    BOOST_CHECK_MESSAGE(tx.IsProposal(), NOT_PROPOSAL_ERROR);

    CVoteProposal proposal2;
    BOOST_CHECK_MESSAGE(ProposalFromTransaction(tx, proposal2), COULD_NOT_DESERIALIZE_ERROR);

    ////////////////////////////////////////////////////////////
    //CHECKS TO DETERMINE IF DESERIALIZATION WORKS AS INTENDED//
    ////////////////////////////////////////////////////////////
    BOOST_CHECK_MESSAGE(proposal2.GetHash() == proposal.GetHash(), HASH_ERROR);
    BOOST_CHECK_MESSAGE(proposal2.GetRefundAddress() == proposal.GetRefundAddress(), REFUND_ADDRESS_ERROR);
    BOOST_CHECK_MESSAGE(proposal2.GetMaxFee() == proposal.GetMaxFee(), MAX_FEE_ERROR);
    BOOST_CHECK_MESSAGE(proposal2.GetBitCount() == proposal.GetBitCount(), BIT_COUNT_ERROR);
    BOOST_CHECK_MESSAGE(proposal2.GetShift() == proposal.GetShift(), SHIFT_ERROR);
    BOOST_CHECK_MESSAGE(proposal2.GetStartHeight() == proposal.GetStartHeight(), START_HEIGHT_ERROR);
    BOOST_CHECK_MESSAGE(proposal2.GetCheckSpan() == proposal.GetCheckSpan(), SPAN_ERROR);
    BOOST_CHECK_MESSAGE(proposal2.GetDescription() == proposal.GetDescription(), DESCRIPTION_ERROR);
    BOOST_CHECK_MESSAGE(proposal2.GetName() == proposal.GetName(), NAME_ERROR);

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
        manager.GetNextLocation(nCardinals, nStartHeight, nCheckSpan, location1);
        CVoteProposal proposal1(strName, nStartHeight, nCheckSpan, strDescription, location1);
        manager.Add(proposal1);

        VoteLocation location2;
        manager.GetNextLocation(nCardinals, nStartHeight, nCheckSpan, location2);
        CVoteProposal proposal2(strName, nStartHeight, nCheckSpan, strDescription, location2);
        manager.Add(proposal2);

        VoteLocation location3;
        manager.GetNextLocation(nCardinals, nStartHeight, nCheckSpan, location3);
        CVoteProposal proposal3(strName, nStartHeight, nCheckSpan, strDescription, location3);
        manager.Add(proposal3);

        VoteLocation location4;
        manager.GetNextLocation(nCardinals, nStartHeight, nCheckSpan, location4);
        CVoteProposal proposal4(strName, nStartHeight, nCheckSpan, strDescription, location4);
        manager.Add(proposal4);

        VoteLocation location5;
        manager.GetNextLocation(nCardinals, nStartHeight, nCheckSpan, location5);
        CVoteProposal proposal5(strName, nStartHeight, nCheckSpan, strDescription, location5);
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
}

BOOST_AUTO_TEST_CASE(vote_deterministic_ordering) {

        std::cout << "testing deterministic ordering of proposal transactions" << endl;

        CVoteProposalManager manager;

        std::string strName = "prop";
        std::string strDescription = "this is the description";
        std::string strRefundAddress = "kxpSiwFkHJdNnYFojNDAvP3iiYyw3GH6ZL";
        int nMaxFee = 5000000;

        CVoteProposal proposal1(strName, nStartHeight, nCheckSpan, strDescription + "1", nMaxFee, strRefundAddress);
        CVoteProposal proposal2(strName, nStartHeight, nCheckSpan, strDescription + "2", nMaxFee, strRefundAddress);
        CVoteProposal proposal3(strName, nStartHeight, nCheckSpan, strDescription + "3", nMaxFee, strRefundAddress);
        CVoteProposal proposal4(strName, nStartHeight, nCheckSpan, strDescription + "4", nMaxFee, strRefundAddress);
        CVoteProposal proposal5(strName, nStartHeight, nCheckSpan, strDescription + "5", nMaxFee, strRefundAddress);

        CTransaction txProposal1;
        BOOST_CHECK_MESSAGE(proposal1.ConstructTransaction(txProposal1), CONSTRUCT_ERROR);

        CTransaction txProposal2;
        BOOST_CHECK_MESSAGE(proposal2.ConstructTransaction(txProposal2), CONSTRUCT_ERROR);

        CTransaction txProposal3;
        BOOST_CHECK_MESSAGE(proposal3.ConstructTransaction(txProposal3), CONSTRUCT_ERROR);

        CTransaction txProposal4;
        BOOST_CHECK_MESSAGE(proposal4.ConstructTransaction(txProposal4), CONSTRUCT_ERROR);

        CTransaction txProposal5;
        BOOST_CHECK_MESSAGE(proposal5.ConstructTransaction(txProposal5), CONSTRUCT_ERROR);

        vector<CTransaction> vTxProposals = {txProposal1, txProposal2, txProposal3, txProposal4, txProposal5};

        vector<CTransaction> vOrderedTxProposals1;
        BOOST_CHECK_MESSAGE(proposalManager.GetDeterministicOrdering(uint256("0xba04fd9cd0e9487f1a713e80828055284c04e362167ceb5f75db70153c613736"),
                                                                      vTxProposals, vOrderedTxProposals1), CREATE_ORDER_ERROR);

        vector<CTransaction> vOrderedTxProposals2;
        BOOST_CHECK_MESSAGE(proposalManager.GetDeterministicOrdering(uint256("0xba04fd9cd0e9487f1a713e80828055284c04e362167ceb5f75db70153c613736"),
                                                                      vTxProposals, vOrderedTxProposals2), CREATE_ORDER_ERROR);

        // verify that both orderings of tx proposals are the same
        for(int i = 0; i < vTxProposals.size(); i++) {
            BOOST_CHECK_MESSAGE(vOrderedTxProposals1.at(i).GetHash() == vOrderedTxProposals2.at(i).GetHash(),
                                DIFFERENT_ORDER_ERROR);
        }

        vector<CTransaction> vDifferentOrderedTxProposals;
        BOOST_CHECK_MESSAGE(proposalManager.GetDeterministicOrdering(uint256("0xd7ec1705209f0212de02409cea9a9ca565571ae2977241f690dac11a7889d3f4"),
                                                 vTxProposals, vDifferentOrderedTxProposals), CREATE_ORDER_ERROR);

        // verify that the ordering of the tx proposals is different for a new proof hash
        bool bSameOrder = true;
        for(int i = 0; i < vTxProposals.size(); i++) {
            bSameOrder = bSameOrder && (vOrderedTxProposals1.at(i).GetHash() == vDifferentOrderedTxProposals.at(i).GetHash());
        }

        BOOST_CHECK_MESSAGE(!bSameOrder, SAME_ORDER_ERROR);
}

BOOST_AUTO_TEST_CASE(vote_refund_process) {
        std::cout << "testing refund process for proposal requests" << endl;

        CVoteProposalManager manager;

        std::string strName = "prop";
        std::string strDescription = "this is the description";
        std::string strRefundAddress = "kxpSiwFkHJdNnYFojNDAvP3iiYyw3GH6ZL";
        int nMaxFee1 = 5000000;
        int nMaxFee2 = 938049809;
        int nMaxFee3 = 3;
        int nMaxFee4 = 1000000000;
        int nMaxFee5 = 6000000;

        CTransaction txCoinBase;
        txCoinBase.vin.resize(1);
        txCoinBase.vin[0].prevout.SetNull();
        txCoinBase.vout.resize(1);

        CVoteProposal proposal1(strName, nStartHeight, nCheckSpan, strDescription, nMaxFee1, strRefundAddress);
        CVoteProposal proposal2(strName, nStartHeight, nCheckSpan, strDescription, nMaxFee2, strRefundAddress);
        CVoteProposal proposal3(strName, nStartHeight, nCheckSpan, strDescription, nMaxFee3, strRefundAddress);
        CVoteProposal proposal4(strName, nStartHeight, nCheckSpan, strDescription, nMaxFee4, strRefundAddress);
        CVoteProposal proposal5(strName, nStartHeight, nCheckSpan, strDescription, nMaxFee5, strRefundAddress);

        CTransaction txProposal1;
        BOOST_CHECK_MESSAGE(proposal1.ConstructTransaction(txProposal1), CONSTRUCT_ERROR);

        CTransaction txProposal2;
        BOOST_CHECK_MESSAGE(proposal2.ConstructTransaction(txProposal2), CONSTRUCT_ERROR);

        CTransaction txProposal3;
        BOOST_CHECK_MESSAGE(proposal3.ConstructTransaction(txProposal3), CONSTRUCT_ERROR);

        CTransaction txProposal4;
        BOOST_CHECK_MESSAGE(proposal4.ConstructTransaction(txProposal4), CONSTRUCT_ERROR);

        CTransaction txProposal5;
        BOOST_CHECK_MESSAGE(proposal5.ConstructTransaction(txProposal5), CONSTRUCT_ERROR);

        vector<CTransaction> vTxProposals = {txProposal1, txProposal2, txProposal3, txProposal4, txProposal5};

        vector<CTransaction> vOrderedTxProposals;
        BOOST_CHECK_MESSAGE(proposalManager.GetDeterministicOrdering(uint256("0xba04fd9cd0e9487f1a713e80828055284c04e362167ceb5f75db70153c613736"),
                                                                      vTxProposals, vOrderedTxProposals), CREATE_ORDER_ERROR);

        for (CTransaction txProposal: vOrderedTxProposals) {
            // output variables
            int nRequiredFee;
            CVoteProposal proposal;
            VoteLocation location;

            // Skip this txProposal if a proposal object cannot be extracted from it
            BOOST_CHECK_MESSAGE(ProposalFromTransaction(txProposal, proposal), CONSTRUCT_ERROR);
            proposalManager.Add(proposal);

            // input variables
            int nTxFee = CVoteProposal::BASE_FEE;
            int nBitCount = proposal.GetBitCount();
            int nStartHeight = proposal.GetStartHeight();
            int nCheckSpan = proposal.GetCheckSpan();

            // If a valid voting location cannot be found then create an unaccepted proposal refund
            if (!proposalManager.GetNextLocation(nBitCount, nStartHeight, nCheckSpan, location)) {
                proposalManager.AddRefundToCoinBase(proposal, nRequiredFee, nTxFee, false, txCoinBase);
            } else {
                BOOST_CHECK(true);
                // If a fee cannot be calculated then skip this proposal without creating a refund tx
                proposal.SetLocation(location);
                BOOST_CHECK_MESSAGE(proposalManager.GetFee(proposal, nRequiredFee), "could not calculate fee.");

                BOOST_CHECK(true);
                // If the maximum fee provided by the proposal creator is less than the required fee
                // then create an unaccepted proposal refund
                if (nRequiredFee > proposal.GetMaxFee()) {
                    BOOST_CHECK(true);
                    proposalManager.AddRefundToCoinBase(proposal, nRequiredFee, nTxFee, false, txCoinBase);
                } else {
                    BOOST_CHECK(true);
                    proposalManager.AddRefundToCoinBase(proposal, nRequiredFee, nTxFee, true, txCoinBase);
                }
            }
        }

        BOOST_CHECK_MESSAGE(proposalManager.CheckRefundTransaction(vOrderedTxProposals, txCoinBase), REFUND_OUT_ERROR);
}

BOOST_AUTO_TEST_SUITE_END()
