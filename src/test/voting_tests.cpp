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

CVoteProposal proposal(strName, nShift, nStartTime, nCheckSpan, nCardinals, strDescription);


BOOST_AUTO_TEST_CASE(proposal_serialization)
{
    std::cout << "testing proposal serialization\n";

    //! Add the constructed proposal to a partial transaction
    CTransaction tx;
    BOOST_CHECK_MESSAGE(proposal.ConstructTransaction(tx), "failed to construct tx");

    BOOST_CHECK_MESSAGE(tx.IsProposal(), "Transaction is not a proposal!");

    CVoteProposal proposal2;
    BOOST_CHECK_MESSAGE(ProposalFromTransaction(tx, proposal2), "Failed to deserialize");

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

BOOST_AUTO_TEST_CASE(vote_tally)
{
    std::cout << "testing vote tally\n";

    map<uint256, VoteLocation> mapNewLocations;
    VoteLocation location;
    location.first = static_cast<uint8_t>(proposal.GetShift() - proposal.GetBitCount());
    location.second = static_cast<uint8_t>(proposal.GetShift());
    mapNewLocations.insert(make_pair(proposal.GetHash(), location));

    CVoteTally tally;
    BOOST_CHECK_MESSAGE(tally.SetNewPositions(mapNewLocations), "Position is already occupied when it should not be");

    uint32_t nVote = 0x40080000;
    // 0000 0000 0000 1000 0000 0000 0000 0000
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
    BOOST_CHECK_MESSAGE(summary4.nNoTally == 1, "summary4 no votes is not 1");

}

BOOST_AUTO_TEST_CASE(vote_charset)
{
//    std::string someString;
//    std::vector<unsigned char> vch;
//    BOOST_CHECK_MESSAGE(ConvertTo6bit("test proposal text", vch), "6 bit character conversion failed");
//    cout << "converted string: " << ReverseEndianString(HexStr(vch)) << endl;
//    BOOST_CHECK_MESSAGE(ConvertTo8bit(vch, someString), "failed to deconvert");
//
//    cout << "deconverted string: " << someString << endl;

        uint32_t nVersion = 0x50000000;
        uint32_t nVote = 2;
        uint256 hash;
        CVoteObject vote(hash, 2, 28);
        vote.Vote(nVote);

        nVersion |= (vote.GetFormattedVote() >> 1);
        cout << "version: " << nVersion << endl;
}

BOOST_AUTO_TEST_SUITE_END()
