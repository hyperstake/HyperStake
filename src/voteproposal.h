// Copyright (c) 2017 The HyperStake Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef VOTEPROPOSAL_H
#define VOTEPROPOSAL_H

#include <iostream>
#include "main.h"
#include "serialize.h"
#include "votetally.h"
#include "voteobject.h"

#define MAX_CHAR_NAME 10
#define MAX_CHAR_ABSTRACT 30
#define MAX_BLOCKS_IN_FUTURE 28800
#define MAX_CHECKSPAN 28800

//TODO: update
#define MOST_RECENT_VERSION 1

class CVoteProposal
{
private:
    // proposal version
    int nVersion;

    // what to call the proposal
    std::string strName;

    // where in the blockchain we start counting votes, nStartHeight
    unsigned int nStartHeight;

    // how far in the blockchain are we scanning
    unsigned int nCheckSpan;

    // the position of the proposal in the nVersion field
    VoteLocation bitLocation;

    // description of the proposal; may link to additional transactions
    std::string strDescription;
public:
    // the amount of HYP burnt when a proposal is made
    static const int64 FEE = 5 * COIN;

    void SetNull()
    {
        nVersion = 0;
        strName = "";
        nStartHeight = 0;
        nCheckSpan = 0;
        strDescription = "";
    }

    bool IsNull () const { return strName.empty(); }

    CVoteProposal()
    {
        SetNull();
    }

    CVoteProposal(std::string strName, unsigned int nStartHeight, unsigned int nCheckSpan, std::string strDescription,
                  VoteLocation location, int nVersion = MOST_RECENT_VERSION)
    {
        this->nVersion = nVersion;
        this->strName = strName;
        this->nStartHeight = nStartHeight;
        this->nCheckSpan = nCheckSpan;
        this->strDescription = strDescription;
        this->bitLocation = location;
    }

    IMPLEMENT_SERIALIZE
    (
       READWRITE(nVersion);
       READWRITE(strName);
       READWRITE(nStartHeight);
       READWRITE(nCheckSpan);
       READWRITE(strDescription);
       READWRITE(bitLocation);
    )

    bool ConstructTransaction (CTransaction& tx) const;
    int GetShift() const { return bitLocation.GetShift(); }
    uint8_t GetBitCount() const { return bitLocation.GetBitCount(); }
    unsigned int GetCheckSpan() const { return nCheckSpan; }
    std::string GetName() const { return strName; }
    std::string GetDescription() const { return strDescription; }
    unsigned int GetStartHeight() const { return nStartHeight; }
    VoteLocation GetLocation() const { return bitLocation; }
    uint256 GetHash() const;

};

bool ProposalFromTransaction(const CTransaction& tx, CVoteProposal& proposal);

#endif //HYPERSTAKE_VOTEPROPOSAL_H
