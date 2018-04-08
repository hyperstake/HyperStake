#include "votetally.h"
#include "util.h"
#include "db.h"
#include "voteobject.h"

#define VOTEMASK 0x0FFFFFFF

using namespace std;

//Convert char* into string showing its binary
string PrintBinary(uint32_t n)
{
    string result;
    for (int i = 0; i < 32; i ++) {
        result.push_back( '0' + (n & 1) );
        n >>= 1;
    }

    reverse( result.begin(), result.end() );

    return result;
}

CVoteTally::CVoteTally(CVoteTally* tallyPrev)
{
    this->nHeight = tallyPrev->nHeight + 1;
    this->mapVotes = tallyPrev->GetVotes();
    RemoveStaleSummaries();
}

void CVoteTally::RemoveStaleSummaries()
{
    for (auto it : mapVotes) {
        //If the vote has ended, then remove it from the new Tally
        if (it.second.nBlockStart + it.second.nCheckSpan <= this->nHeight) {
            mapLocations.erase(it.first);
            mapVotes.erase(it.first);
        }
    }
}

//! Proposal Manager will give a set of new positions if any start this block
bool CVoteTally::SetNewPositions(std::map<uint256, VoteLocation> &mapNewLocations)
{
    mapLocations.clear();
    for (auto it : mapNewLocations) {
        if (mapLocations.count(it.first))
            continue;

        uint256 txid = 0;
        if (!GetProposalTXID(it.first, txid))
            return error("%s: could not find transaction ID for proposal %s", __func__, it.first.GetHex().c_str());

        CVoteProposal proposal;
        CVoteDB votedb("r");
        if (!votedb.ReadProposal(txid, proposal))
            return error("%s: failed to read proposal from DB for %s", __func__, it.first.GetHex().c_str());

        //Map the proposal to this bit range
        VoteLocation locationNew = it.second;
        mapLocations.insert(make_pair(it.first, locationNew));

        //Start a new summary object that will track the votes for this proposal
        CVoteSummary summary;
        summary.nBlockStart = proposal.GetStartHeight();
        summary.nCheckSpan = proposal.GetCheckSpan();
        mapVotes.insert(make_pair(it.first, summary));
    }

    return true;
}

//! Record votes that were in the block header
void CVoteTally::ProcessNewVotes(const uint32_t& nVersion)
{
    for (auto it : mapVotes) {
        if (!mapLocations.count(it.first))
            continue;

        printf("%s processing vote for %s\n", __func__, it.first.GetHex().c_str());

        VoteLocation location = mapLocations.at(it.first);
        int32_t nVote = nVersion;
        nVote &= VOTEMASK; // remove version bits
        nVote >>= location.GetShift(); //shift it over to the starting position
        int32_t nBits = location.GetBitCount();

        // Remove any bits to the left of the vote bits
        uint32_t nMask = ~0;
        nMask >>= (32 - nBits);
        nVote &= nMask;

        //Count the vote if it is yes or no
        if (nVote == 1) {
            mapVotes.at(it.first).nYesTally++;
        } else if (nVote == 2)
            mapVotes.at(it.first).nNoTally++;
        printf("%s: nVote=%d\n", __func__, nVote);
    }
}

bool CVoteTally::GetSummary(const uint256& hashProposal, CVoteSummary& summary)
{
    auto it = mapVotes.find(hashProposal);
    if (it == mapVotes.end())
        return false;

    summary = it->second;
    return true;
}



