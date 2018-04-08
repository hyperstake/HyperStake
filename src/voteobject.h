// Copyright (c) 2017 The HyperStake Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef HYPERSTAKE_VOTEOBJECT_H
#define HYPERSTAKE_VOTEOBJECT_H

#include "serialize.h"
#include "uint256.h"

class VoteLocation
{
public:
    //bit positions increase from right to left, starting at position 0
    uint8_t nMostSignificantBit;
    uint8_t nLeastSignificantBit;

    void SetNull()
    {
        nMostSignificantBit = 0;
        nLeastSignificantBit = 0;
    }

    VoteLocation(){
        SetNull();
    }

    VoteLocation(uint8_t nMostSignificantBit, uint8_t nLeastSignificantBit)
    {
        this->nMostSignificantBit = nMostSignificantBit;
        this->nLeastSignificantBit = nLeastSignificantBit;
    }

    IMPLEMENT_SERIALIZE
    (
            READWRITE(nMostSignificantBit);
            READWRITE(nLeastSignificantBit);
    )

    uint8_t GetBitCount() const { return (uint8_t)(nMostSignificantBit - nLeastSignificantBit + 1); }
    uint8_t GetShift() const { return nLeastSignificantBit; }
};

class CVoteObject
{
private:
    uint32_t nChoice;
    uint32_t nFormattedVote;
    bool fVoted;
    uint256 hashProposal;
    VoteLocation bitLocation;
public:

    static int GetCombinedVotes(const std::vector<CVoteObject>& votes);

    void SetNull()
    {
        nChoice = 0;
        nFormattedVote = 0;
        fVoted = false;
        hashProposal = 0;
    }

    bool IsNull () { return hashProposal == 0; }

    CVoteObject()
    {
        SetNull();
    }

    CVoteObject(uint256 hashProposal, VoteLocation location)
    {
        SetNull();
        this->hashProposal = hashProposal;
        this->bitLocation = location;
    }

    IMPLEMENT_SERIALIZE
    (
            READWRITE(nChoice);
            READWRITE(nFormattedVote);
            READWRITE(fVoted);
            READWRITE(hashProposal);
            READWRITE(bitLocation);
    )

    bool Vote(int nVotersChoice);

    //The vote value shifted to the position of the proposal, use staking a vote
    uint32_t GetFormattedVote() { return (nChoice << bitLocation.GetShift()); }

    //The unshifted vote value defined in ProcessVote
    //POTENTIAL TODO: create enum to represent choice
    uint32_t GetUnformattedVote() { return nChoice; }
};

#endif //HYPERSTAKE_VOTEOBJECT_H
