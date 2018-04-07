// Copyright (c) 2017 The HyperStake Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef HYPERSTAKE_VOTEOBJECT_H
#define HYPERSTAKE_VOTEOBJECT_H

#include "serialize.h"
#include "uint256.h"

class CVoteObject
{
private:
    uint32_t nChoice;
    uint32_t nFormattedVote;
    bool fVoted;
    uint256 hashProposal;
    int nCardinals;
    int nShift;
public:

    void SetNull()
    {
        nChoice = 0;
        nFormattedVote = 0;
        fVoted = false;
        hashProposal = 0;
        nCardinals = 0;
        nShift = 0;
    }

    bool IsNull () { return hashProposal == 0; }

    CVoteObject()
    {
        SetNull();
    }

    CVoteObject(uint256 hashProposal, int nCardinals, int nShift)
    {
        SetNull();
        this->hashProposal = hashProposal;
        this->nCardinals = nCardinals;
        this->nShift = nShift;
    }

    IMPLEMENT_SERIALIZE
    (
            READWRITE(nChoice);
            READWRITE(nFormattedVote);
            READWRITE(fVoted);
            READWRITE(hashProposal);
            READWRITE(nCardinals);
            READWRITE(nShift);
    )

    bool Vote(int nVotersChoice);
    uint32_t GetFormattedVote() { return (nChoice << nShift); }
    uint32_t GetVote() { return nChoice; }
};

#endif //HYPERSTAKE_VOTEOBJECT_H
