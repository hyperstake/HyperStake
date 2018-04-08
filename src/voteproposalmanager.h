// Copyright (c) 2017 The HyperStake Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HYPERSTAKE_VOTEPROPOSALMANAGER_H
#define HYPERSTAKE_VOTEPROPOSALMANAGER_H

#include <set>
#include "voteobject.h"

class CVoteProposal;

struct CProposalMetaData
{
    uint256 hash;
    VoteLocation location;
    unsigned int nHeightStart;
    unsigned int nHeightEnd;
};

class CVoteProposalManager
{
private:
    std::map<uint256, CProposalMetaData> mapProposalData;
public:
    bool Add(const CVoteProposal& proposal);
    void Remove(const uint256& hashProposal);
    std::map<uint256, VoteLocation> GetActive(int nHeight);
    bool GetNextLocation(int nBitCount, int nStartHeight, int nCheckSpan, VoteLocation& location);
    std::map<uint256, CProposalMetaData> GetAllProposals() const { return mapProposalData; };
};

#endif //HYPERSTAKE_VOTEPROPOSALMANAGER_H
