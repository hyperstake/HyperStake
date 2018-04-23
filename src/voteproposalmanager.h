// Copyright (c) 2017 The HyperStake Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HYPERSTAKE_VOTEPROPOSALMANAGER_H
#define HYPERSTAKE_VOTEPROPOSALMANAGER_H

#include <set>
#include "voteobject.h"

class CVoteProposal;

//1. calculate fee for proposal
//2. fee is a function of bits used and range with a restriction on start time
//3. the input to the fee function is determined by most overlapped section of range

struct CProposalMetaData
{
    uint256 hash;
    VoteLocation location;

    //this range must be inclusive
    unsigned int nHeightStart;
    unsigned int nHeightEnd;
};

class CVoteProposalManager
{
private:
    std::map<uint256, CProposalMetaData> mapProposalData;
public:
    static const int MAX_SPAN = -1; //TODO: set
    static const int MAX_DISTANCE_TO_START = -1; //TODO: set

    static bool isProposalValid()

    bool Add(const CVoteProposal& proposal);
    void Remove(const uint256& hashProposal);
    std::map<uint256, VoteLocation> GetActive(int nHeight);
    bool GetNextLocation(int nBitCount, int nStartHeight, int nCheckSpan, VoteLocation& location);
    std::map<uint256, CProposalMetaData> GetAllProposals() const { return mapProposalData; };
    bool CheckProposal (const CVoteProposal& proposal);
};

#endif //HYPERSTAKE_VOTEPROPOSALMANAGER_H
