// Copyright (c) 2017 The HyperStake Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "voteproposalmanager.h"
#include "voteproposal.h"

using namespace std;

//! Add a proposal to the manager. Note that it must not have conflicts in its scheduling.
bool CVoteProposalManager::Add(const CVoteProposal& proposal)
{
    CProposalMetaData newProposal;
    newProposal.location = proposal.GetLocation();
    newProposal.hash = proposal.GetHash();
    newProposal.nHeightStart = proposal.GetStartHeight();
    newProposal.nHeightEnd = newProposal.nHeightStart + proposal.GetCheckSpan();

    //Check if any of the existing proposals are using the same bits during the same time
    for (auto it : mapProposalData) {
        CProposalMetaData existingProposal = it.second;
        //Clear of any conflicts, starts after the existing proposal ends
        if (newProposal.location.nMostSignificantBit < existingProposal.location.nLeastSignificantBit)
            continue;
        //Clear of any conflicts, ends before the existing proposal starts
        if (newProposal.location.nLeastSignificantBit > existingProposal.location.nMostSignificantBit)
            continue;
        //Clear of any conflicts, there is not overlap in the voting period
        if (newProposal.nHeightStart > existingProposal.nHeightEnd || newProposal.nHeightEnd < existingProposal.nHeightStart)
            continue;

        return error("%s: Proposal position is already occupied during the block span requested", __func__);
    }

    mapProposalData.insert(make_pair(newProposal.hash, newProposal));
    printf("%s: added proposal %s\n", __func__, newProposal.hash.GetHex().c_str());
    return true;
}

//! Remove a proposal from the proposal manager
void CVoteProposalManager::Remove(const uint256& hashProposal)
{
    auto it = mapProposalData.find(hashProposal);
    if (it != mapProposalData.end())
        mapProposalData.erase(it);
}

//! Get proposals that are actively being voted on
map<uint256, VoteLocation> CVoteProposalManager::GetActive(int nHeight)
{
    map<uint256, VoteLocation> mapActive;
    for (auto it : mapProposalData) {
        CProposalMetaData data = it.second;
        if ((int)data.nHeightStart <= nHeight && (int)data.nHeightEnd >= nHeight)
            mapActive.insert(make_pair(data.hash, data.location));
    }

    return mapActive;
}

namespace
{
    //returns the maximum number of proposals overlapping at any point within the given range
    int GetMaxOverlap(const vector<CProposalMetaData>& vProposals, const unsigned int& nStart, const unsigned int& nEnd)
    {
        int nMaxOverlapQuantity;
        vector<int> vOverlapCounter(nEnd - nStart + 1);

        for(auto proposalData: vProposals) {
            if(proposalData.nHeightEnd < nStart) continue;
            if(proposalData.nHeightStart > nEnd) continue;

            vOverlapCounter.at(proposalData.nHeightStart - nStart)++;
            vOverlapCounter.at(proposalData.nHeightEnd - nStart)++;
        }

        int nCurValueCounter = 0;
        for(int count: vOverlapCounter) {
            nCurValueCounter += count;
            nMaxOverlapQuantity = max(nMaxOverlapQuantity, nCurValueCounter);
        }

        return nMaxOverlapQuantity;
    }

    //returns a vector of proposals that overlap with the given range
    vector<CProposalMetaData> GetOverlappingProposals(const map<uint256, CProposalMetaData>& mapProposalData,
                                                      const int& nStart, const int& nEnd)
    {
        vector<CProposalMetaData> vConflictingTime;
        for (auto it : mapProposalData) {
            CProposalMetaData data = it.second;
            if ((int)data.nHeightEnd < nStart)
                continue;
            if ((int)data.nHeightStart > nEnd)
                continue;
            vConflictingTime.emplace_back(data);
        }
    }
}

bool CVoteProposalManager::GetNextLocation(int nBitCount, int nStartHeight, int nCheckSpan, VoteLocation& location)
{
    //Conflicts for block range
    vector<CProposalMetaData> vConflictingTime = GetOverlappingProposals(mapProposalData, nStartHeight, nStartHeight + nCheckSpan - 1);

    //Maximum number of conflicts for block range
    int nMaxConflict = GetMaxOverlap(vConflictingTime, nStartHeight, nStartHeight + nCheckSpan - 1);

    //Find an open location for the new proposal, return left most bits
    if (vConflictingTime.empty()) {
        location.nMostSignificantBit = 27;
        location.nLeastSignificantBit = (uint8_t)(location.nMostSignificantBit - nBitCount + 1);
        return true;
    }

    //create a vector tracking available spots
    vector<int> vAvailable(28, 1);

    //remove spots that are already taken
    for (auto data : vConflictingTime) {
        for (int i = data.location.nMostSignificantBit; i >= data.location.nLeastSignificantBit; i--) {
            vAvailable.at(i) = 0;
        }
    }

    //find an available sequence of bits that fit the proposal
    vector<int> vRange;
    int nSequential = 0;
    for (int i = 27; i >= 0; i--) {
        uint8_t n = static_cast<uint8_t>(i);
        nSequential = vAvailable.at(n) == 1 ? nSequential + 1 : 0;
        if (nSequential == nBitCount) {
            location.nLeastSignificantBit = n;
            location.nMostSignificantBit = static_cast<uint8_t>(n + nBitCount - 1);
            return true;
        }
    }
    return false;
}