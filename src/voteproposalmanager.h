// Copyright (c) 2017 The HyperStake Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef HYPERSTAKE_VOTEPROPOSALMANAGER_H
#define HYPERSTAKE_VOTEPROPOSALMANAGER_H

#include <set>
#include "voteobject.h"

class CVoteProposal;
class CTransaction;
class CBitcoinAddress;

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

    bool Add(const CVoteProposal& proposal);
    void Remove(const uint256& hashProposal);
    std::map<uint256, VoteLocation> GetActive(int nHeight);

    //methods used in dynamic fee calculation
    bool GetFee(const CVoteProposal& proposal, int& nFee);
    bool GetDeterministicOrdering(const uint256& proofhash, std::vector<CTransaction> vProposalTransactions,
                                    std::vector<CTransaction>& vOrderedProposalTransactions);
    bool GetNextLocation(int nBitCount, int nStartHeight, int nCheckSpan, VoteLocation& location);
    bool AddRefundToCoinBase(const CVoteProposal &proposal, const int &nRequiredFee, const int &nTxFee,
                             const bool bProposalAccepted, CTransaction &txCoinBase);
    bool CheckRefundTransaction(const std::vector<CTransaction> &vOrderedTxProposals, const CTransaction &txCoinBase);
    bool GetAcceptedTxProposals(const CTransaction& txCoinBase, const std::vector<CTransaction>& vOrderedTxProposals,
                                std::vector<CTransaction>& vAcceptedTxProposals);
    bool GetRefundOutputSize(const CTransaction& txProposal, int& nSize) const;

    std::map<uint256, CProposalMetaData> GetAllProposals() const { return mapProposalData; };
};

#endif //HYPERSTAKE_VOTEPROPOSALMANAGER_H
