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
        if (newProposal.location.first < existingProposal.location.second)
            continue;
        //Clear of any conflicts, ends before the existing proposal starts
        if (newProposal.location.second > existingProposal.location.first)
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

bool CVoteProposalManager::GetNextLocation(int nBitCount, int nStartHeight, int nCheckSpan, VoteLocation& location)
{
    //Conflicts for block range
    vector<CProposalMetaData> vConflictingTime;
    for (auto it : mapProposalData) {
        CProposalMetaData data = it.second;
        int nEndHeight = nStartHeight + nCheckSpan;
        if (data.nHeightEnd < nStartHeight)
            continue;
        if (data.nHeightStart > nEndHeight)
            continue;
        vConflictingTime.emplace_back(data);
    }

    //Find an open location for the new proposal, return left most bits
    if (vConflictingTime.empty()) {
        location.first = 28;
        location.second = (uint8_t)(location.first - nBitCount + 1);
        return true;
    }

    //create a vector tracking available spots
    vector<int> vAvailable(29, 1);

    //remove spots that are already taken
    for (auto data : vConflictingTime) {
        printf("location.first=%d location.second=%d\n", location.first, location.second);
        for (int i = data.location.first; i >= data.location.second; i--) {
            printf("mark %d as 0\n", i);
            vAvailable.at(i) = 0;
        }
    }

    //find an available sequence of bits that fit the proposal
    vector<int> vRange;
    int nSequential = 0;
    for (uint8_t i = 28; i > 0; i--) {
        nSequential = vAvailable.at(i) == 1 ? nSequential + 1 : 0;
        if (nSequential == nBitCount) {
            location.second = i;
            location.first = (uint8_t )(i + nBitCount - 1);
            return true;
        }
    }
    return false;
}