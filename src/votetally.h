#ifndef VOTING_VOTETALLY_H
#define VOTING_VOTETALLY_H

#include <map>
#include "serialize.h"
#include "uint256.h"

class VoteLocation;

class CVoteSummary
{
public:
    unsigned int nBlockStart;
    unsigned int nCheckSpan;
    unsigned int nYesTally;
    unsigned int nNoTally; //Since voters can abstain, a no tally is needed

    CVoteSummary()
    {
        nBlockStart = 0;
        nCheckSpan = 0;
        nYesTally = 0;
        nNoTally = 0;
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(nBlockStart);
        READWRITE(nCheckSpan);
        READWRITE(nYesTally);
        READWRITE(nNoTally);
    )
};

//typedef std::pair<uint8_t, uint8_t> VoteLocation;  //start bit, end bit

class CVoteTally
{
private:
    unsigned int nHeight;
    std::map<uint256, CVoteSummary> mapVotes;
    std::map<uint256, VoteLocation> mapLocations; //Where each vote is located in the header
    void RemoveStaleSummaries();
    bool IsLocationOccupied(const VoteLocation& location);
public:
    void SetNull()
    {
        nHeight = 0;
        mapVotes.clear();
        mapLocations.clear();
    }

    CVoteTally()
    {
        SetNull();
    }

    explicit CVoteTally(CVoteTally* tallyPrev);
    bool SetNewPositions(std::map<uint256, VoteLocation>& mapNewLocations);
    void ProcessNewVotes(const uint32_t& nVersion);
    bool GetSummary(const uint256& hashProposal, CVoteSummary& summary);
    std::map<uint256, CVoteSummary> GetVotes() { return mapVotes; }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(nHeight);
        READWRITE(mapVotes);
        READWRITE(mapLocations);
    )
};

#endif //VOTING_VOTETALLY_H

