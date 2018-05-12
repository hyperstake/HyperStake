// Copyright (c) 2017 The HyperStake Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "voteproposalmanager.h"
#include "voteproposal.h"
#include "base58.h"

using namespace std;

bool CVoteProposalManager::CheckProposal(const CVoteProposal &proposal)
{
    // If the proposal is already in the blockchain then it's guaranteed to be valid
    if (mapProposalData.count(proposal.GetHash()) != 0) {
        return true;
    }

    // Proposal name length must be between 1 and MAX_CHAR_NAME (inclusive)
    if (proposal.GetName().empty() || proposal.GetName().size() > MAX_CHAR_NAME) {
        return false;
    }

    // Proposal description length must be between 1 and MAX_CHAR_ABSTRACT (inclusive)
    if (proposal.GetDescription().empty() || proposal.GetDescription().size() > MAX_CHAR_ABSTRACT) {
        return false;
    }

    // Proposal voting period cannot start before or at the current height or after MAX_BLOCKS_IN_FUTURE
    if (proposal.GetStartHeight() <= nBestHeight || proposal.GetStartHeight() > nBestHeight + MAX_BLOCKS_IN_FUTURE) {
        return false;
    }

    // Proposal voting period length must be between 1 and MAX_CHECKSPAN (inclusive)
    if (!proposal.GetCheckSpan() || proposal.GetCheckSpan() > MAX_CHECKSPAN) {
        return false;
    }

    // Check to see if there is room on the blockchain for this proposal
    VoteLocation location;
    if (!GetNextLocation(proposal.GetBitCount(), proposal.GetStartHeight(), proposal.GetCheckSpan(), location)) {
        return false;
    }

    return true;
}

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
    //An Event is either the beginning or end of a vote proposal span.
    //This struct is used for GetMaxOverlap
    struct Event
    {
        bool start;
        int position;
        int bitCount;

        //default constructor
        Event() {}

        Event(bool start, int position, int bitCount = 0)
        {
            this->start = start;
            this->position = position;
            this->bitCount = bitCount;
        }

        static bool Compare(const Event& lhs, const Event& rhs)
        {
            return lhs.position < rhs.position;
        }
    };

    //returns the maximum number of proposals overlapping at any point within the given range
    /*unsigned int GetMaxOverlap(const vector<CProposalMetaData>& vProposals, const unsigned int& nStart, const unsigned int& nEnd)
    {
        int nMaxOverlapQuantity = -1;
        vector<Event> vEvents(2 * vProposals.size());

        for(auto proposalData: vProposals) {
            if(proposalData.nHeightEnd < nStart) continue;
            if(proposalData.nHeightStart > nEnd) continue;

            vEvents.emplace_back(Event(true, proposalData.nHeightStart));
            vEvents.emplace_back(Event(false, proposalData.nHeightEnd));
        }

        sort(vEvents.begin(), vEvents.end(), Event::Compare);

        int nCurValueCounter = 0;
        for(Event event: vEvents) {
            nCurValueCounter += event.start ? 1 : -1;
            nMaxOverlapQuantity = max(nMaxOverlapQuantity, nCurValueCounter);
        }

        return (unsigned int)nMaxOverlapQuantity;
    }*/

    long GetResourceUsageHeuristic(const vector<CProposalMetaData>& vProposals, const CVoteProposal& proposal)
    {
        long nHeuristic = 0;
        int nStart = proposal.GetStartHeight();
        int nEnd = proposal.GetStartHeight() + proposal.GetCheckSpan();

        // An event is defined as either a proposal interval start or a proposal interval ending.
        vector<Event> vEvents(2 * vProposals.size());

        // For each proposal in vProposals that overlaps with the given proposal, create a start and end event then
        // add it to vEvents. This vector will be used to determine the number of overlapping voting intervals efficiently.
        for(auto proposalData: vProposals) {
            if(proposalData.nHeightEnd < nStart) continue;
            if(proposalData.nHeightStart > nEnd) continue;

            Event startEvent(true, proposalData.nHeightStart, proposalData.location.GetBitCount());
            Event endEvent(false, proposalData.nHeightEnd + 1, proposalData.location.GetBitCount());

            vEvents.emplace_back(startEvent);
            vEvents.emplace_back(endEvent);
        }

        // sort the events so that those that happen earlier appear first in the vector
        sort(vEvents.begin(), vEvents.end(), Event::Compare);

        // iterate through events in sorted order and keep a running counter of how many bits are consumed
        int nCurValueCounter = 0;
        for(unsigned int i = 0; i < vEvents.size() - 1; i++) {
            Event curEvent = vEvents.at(i);
            Event nextEvent = vEvents.at(i + 1);

            nCurValueCounter += curEvent.start ? curEvent.bitCount : -1 * curEvent.bitCount;

            // only start the counter when we have entered the voting intervals of the given proposal
            if(nextEvent.position <= nStart) continue;
            if(curEvent.position > nEnd) break;

            // the number of bits used is guaranteed to be constant for every block between these two events
            int gap = min(nEnd, nextEvent.position) - max(nStart, curEvent.position);

            // TODO: heuristic updated; this is subject to change
            nHeuristic += (100000 * ((long) proposal.GetBitCount())) / (MAX_BITCOUNT - nCurValueCounter) * gap;
        }

        return nHeuristic;
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

        return vConflictingTime;
    }
}

//TODO: test fee output for different inputs. Might change this to grow exponentially
bool CVoteProposalManager::GetFee(const CVoteProposal& proposal, int& nFee)
{
    if(!proposal.IsValid()){
        return error("Proposal is not valid");
    }

    //set the boundaries of the voting interval
    unsigned int nStartHeight = proposal.GetStartHeight();
    unsigned int nEndHeight = nStartHeight + proposal.GetCheckSpan() - 1;

    //get conflicting proposals
    vector<CProposalMetaData> vConflictingTime = GetOverlappingProposals(mapProposalData, nStartHeight, nEndHeight);

    //determine the maximum number of overlapping proposals at any point in the voting interval
    long nHeuristic = GetResourceUsageHeuristic(vConflictingTime, proposal) / 100000;

    //TODO: this will need to be tested
    nFee = (unsigned int)(nHeuristic * CVoteProposal::BASE_FEE);

    if(nFee < 0) {
        return error("Fee should not be negative");
    }

    return true;
}

bool CVoteProposalManager::GetDeterministicOrdering(const uint256 &proofhash, std::vector<CTransaction> &vProposalTransactions,
                                                    std::vector<CTransaction> &vOrderedProposalTransactions)
{
    int nMask = 0x000FFFFF;
    int nSegmentSize = 20;
    int nSegmentOffset = 0;
    while(!vProposalTransactions.empty()) {
        uint256 nFormattedMask = nMask << (nSegmentOffset * nSegmentSize);
        int segment = (int)((proofhash & nFormattedMask) >> (nSegmentOffset * nSegmentOffset)).Get64();
        int index = (int)(segment % vProposalTransactions.size());

        if(segment < 0 || index < 0) {
            return error("Generated index is invalid");
        }

        vOrderedProposalTransactions.emplace_back(vProposalTransactions.at(index));
        vProposalTransactions.erase(vProposalTransactions.begin() + index);

        nSegmentOffset = (nSegmentOffset + nSegmentSize) % 256;
    }

    return true;
}

//TODO: MAX TRANSACTION SIZE CHECK
bool CVoteProposalManager::AddRefundToCoinBase(const CVoteProposal &proposal, const int &nRequiredFee, const int &nTxFee,
                                               const bool bProposalAccepted, CTransaction &txCoinBase)
{
    if (!txCoinBase.IsCoinBase()) {
        return error("AddRefundToCoinBase() : Given transaction is the a coinbase transaction.");
    }

    CBitcoinAddress refundAddress;
    if (!refundAddress.SetString(proposal.GetRefundAddress())) {
        return error("AddRefundToCoinBase() : Refund Address of proposal is not valid");
    }

    CTxOut refundTxOut;
    refundTxOut.nValue = bProposalAccepted ? proposal.GetMaxFee() - nRequiredFee - nTxFee : proposal.GetMaxFee() - nTxFee;
    refundTxOut.scriptPubKey.SetDestination(refundAddress.Get());
    txCoinBase.vout.emplace_back(refundTxOut);

    return true;
}

//TODO: REFACTOR AND COMMENT
bool CVoteProposalManager::CheckRefundTransaction(const std::vector<CTransaction> &vOrderedTxProposals,
                                                  const CTransaction &txCoinBase)
{
    if (!txCoinBase.IsCoinBase()) {
        return error("CheckRefundTransaction() : Given transaction is not a coinbase.");
    }

    CTransaction txExpectedCoinBase;

    for(auto txProposal: vOrderedTxProposals) {
        // output variables
        int nRequiredFee;
        CVoteProposal proposal;
        VoteLocation location;

        // return error if a proposal object cannot be extracted from the tx
        if(!ProposalFromTransaction(txProposal, proposal)) {
            return error("CheckRefundTransaction() : Proposal was not able to be extracted from transaction.");
        }

        // input variables
        int nTxFee = (int)CVoteProposal::BASE_FEE; //TODO: MAKE THIS HIGHER
        int nBitCount = proposal.GetBitCount();
        int nStartHeight = proposal.GetStartHeight();
        int nCheckSpan = proposal.GetCheckSpan();

        // If a valid voting location cannot be found then create an unaccepted proposal refund
        if(!GetNextLocation(nBitCount, nStartHeight, nCheckSpan, location)) {
            AddRefundToCoinBase(proposal, nRequiredFee, nTxFee, false, txExpectedCoinBase);
        } else {
            // If a fee cannot be calculated then return error
            proposal.SetLocation(location);
            if (!GetFee(proposal, nRequiredFee)) {
                return error("CheckRefundTransaction() : Calculating fee for proposal failed.");
            }

            // If the maximum fee provided by the proposal creator is less than the required fee
            // then create an unaccepted proposal refund
            if (nRequiredFee > proposal.GetMaxFee()) {
                AddRefundToCoinBase(proposal, nRequiredFee, nTxFee, false, txExpectedCoinBase);
            } else {
                AddRefundToCoinBase(proposal, nRequiredFee, nTxFee, true, txExpectedCoinBase);
            }
        }
    }

    for(int i = 0; i < txCoinBase.vout.size(); i++) {
        if(txCoinBase.vout.at(i).scriptPubKey.GetID().GetHex() != txExpectedCoinBase.vout.at(i).scriptPubKey.GetID().GetHex()) {
            return error("CheckRefundTransaction() : The scriptPubKey of the refund transaction isn't what it should be"
                         "according to the deterministic ordering.");
        }

        if(txCoinBase.vout.at(i).nValue != txExpectedCoinBase.vout.at(i).nValue) {
            return error("CheckRefundTransaction() : The value of the refund isn't what it should be according to the"
                         "deterministic ordering.");
        }
    }

    return true;
}

bool CVoteProposalManager::GetAcceptedTxProposals(const CTransaction& txCoinBase, const std::vector<CTransaction>& vOrderedTxProposals,
                          std::vector<CTransaction>& vAcceptedTxProposals)
{
    if (!txCoinBase.IsCoinBase()) {
        return error("GetAcceptedTxProposals() : Given transaction is not a coinbase.");
    }

    vAcceptedTxProposals.clear();

    for(auto txProposal: vOrderedTxProposals) {
        // output variables
        int nRequiredFee;
        CVoteProposal proposal;
        VoteLocation location;

        // return error if a proposal object cannot be extracted from the tx
        if(!ProposalFromTransaction(txProposal, proposal)) {
            return error("GetAcceptedTxProposals() : Proposal was not able to be extracted from transaction.");
        }

        // input variables
        int nBitCount = proposal.GetBitCount();
        int nStartHeight = proposal.GetStartHeight();
        int nCheckSpan = proposal.GetCheckSpan();

        if(GetNextLocation(nBitCount, nStartHeight, nCheckSpan, location)){
            // If a fee cannot be calculated then return error
            proposal.SetLocation(location);
            if (!GetFee(proposal, nRequiredFee)) {
                return error("GetAcceptedTxProposals() : Calculating fee for proposal failed.");
            }

            // If the max fee provided by the txProposal exceeds the required fee the accept the tx as a valid proposal
            if (nRequiredFee >= proposal.GetMaxFee()) {
                vAcceptedTxProposals.emplace_back(txProposal);
            }
        }
    }

    return true;
}

bool CVoteProposalManager::GetNextLocation(int nBitCount, int nStartHeight, int nCheckSpan, VoteLocation& location)
{
    //Conflicts for block range
    vector<CProposalMetaData> vConflictingTime = GetOverlappingProposals(mapProposalData, nStartHeight, nStartHeight + nCheckSpan - 1);

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

bool CVoteProposalManager::GetRefundOutputSize(const CTransaction& txProposal, int& nSize) const
{

    CTransaction txEmpty;
    unsigned int nBaseSize = ::GetSerializeSize(txEmpty, SER_NETWORK, PROTOCOL_VERSION);

    if (!txProposal.IsProposal()) {
        return error("GetRefundOutputSize() : Given transaction must be a proposal.");
    }

    CVoteProposal proposal;
    if (!ProposalFromTransaction(txProposal, proposal)) {
        return error("GetRefundOutputSize() : Failed to extract proposal from transaction.");
    }

    // Every refund output should increase the size of the coinbase tx by the same amount.
    // 0, 0, and false are just filler values.
    proposalManager.AddRefundToCoinBase(proposal, 0, 0, false, txEmpty);

    nSize = ::GetSerializeSize(txEmpty, SER_NETWORK, PROTOCOL_VERSION) - nBaseSize;

    return true;
}
