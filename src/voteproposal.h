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
#define MAX_BITCOUNT 28

//TODO: update
#define MOST_RECENT_VERSION 1

class CVoteProposal
{
private:
    // proposal version
    int nVersion;

    // proposal max fee provided by creator of proposal
    int nMaxFee;

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

    // refund address
    std::string strRefundAddress;
public:
    // the amount of HYP burnt when a proposal is made
    static const int64 BASE_FEE = 5 * COIN;

    void SetNull()
    {
        nVersion = 0;
        nMaxFee = 0;
        strName = "";
        nStartHeight = 0;
        nCheckSpan = 0;
        strDescription = "";
        strRefundAddress = "";
    }

    bool IsNull () const { return strName.empty(); }

    CVoteProposal()
    {
        SetNull();
    }

    CVoteProposal(std::string strName, unsigned int nStartHeight, unsigned int nCheckSpan, std::string strDescription,
                  int nMaxFee, std::string strRefundAddress, int nVersion = MOST_RECENT_VERSION)
    {
        this->nVersion = nVersion;
        this->strName = strName;
        this->nStartHeight = nStartHeight;
        this->nCheckSpan = nCheckSpan;
        this->strDescription = strDescription;
        this->nMaxFee = nMaxFee;
        this->strRefundAddress = strRefundAddress;

        //VoteLocation will be set when the proposal is accepted by the network and the dynamic fee is determined
    }

    //DEPRECATED: USED FOR TESTING
    CVoteProposal(std::string strName, unsigned int nStartHeight, unsigned int nCheckSpan, std::string strDescription,
                  VoteLocation location, int nVersion = MOST_RECENT_VERSION)
    {
        SetNull();
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
       READWRITE(nMaxFee);
       READWRITE(strName);
       READWRITE(nStartHeight);
       READWRITE(nCheckSpan);
       READWRITE(strDescription);
       READWRITE(bitLocation);
       READWRITE(strRefundAddress);
    )

    bool IsValid() const;
    bool ConstructTransaction (CTransaction& tx) const;
    int GetShift() const { return bitLocation.GetShift(); }
    uint8_t GetBitCount() const { return bitLocation.GetBitCount(); }
    unsigned int GetCheckSpan() const { return nCheckSpan; }
    std::string GetName() const { return strName; }
    std::string GetDescription() const { return strDescription; }
    unsigned int GetStartHeight() const { return nStartHeight; }
    VoteLocation GetLocation() const { return bitLocation; }
    int GetMaxFee() const { return nMaxFee; }
    std::string GetRefundAddress() const { return strRefundAddress; }
    uint256 GetHash() const;

    void SetLocation(VoteLocation location) { this->bitLocation = location; }
};

bool ProposalFromTransaction(const CTransaction& tx, CVoteProposal& proposal);

#endif //HYPERSTAKE_VOTEPROPOSAL_H
