// Copyright (c) 2017 The HyperStake Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "voteproposal.h"

using namespace std;

uint256 CVoteProposal::GetHash() const
{
    return SerializeHash(*this);
}

/**
* The vote proposal is serialized and added to a CTransaction as a data object via OP_RETURN transaction output.
* The transaction is marked as a proposal by marking the first 4 bytes as "PROP" in ASCII
*/
bool CVoteProposal::ConstructTransaction (CTransaction& tx) const
{
//    if (tx.vin.empty()) {
//        printf("%s : transaction does not have any inputs!\n", __func__);
//        return false;
//    }

    if (IsNull()) {
        printf("%s : vote proposal is null!\n", __func__);
        return false;
    }

    //serialize the vote proposal
    CDataStream serializedProposal(SER_NETWORK, 0);
    serializedProposal << *this;

    //Construct the script that will include the serialized proposal
    CScript scriptProposal;
    vector<unsigned char> vchMessage;
    vchMessage.push_back(0x70); // P
    vchMessage.push_back(0x72); // R
    vchMessage.push_back(0x6f); // O
    vchMessage.push_back(0x70); // P
    vchMessage.insert(vchMessage.end(), serializedProposal.begin(), serializedProposal.end());
    scriptProposal << OP_RETURN << vchMessage;

    //Create txout and add it to the transaction
    CTxOut out;
    out.scriptPubKey = scriptProposal;
    out.nValue = 0;
    tx.vout.push_back(out);

    if (tx.GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION) > MAX_BLOCK_SIZE) {
        printf("%s : transaction size is too large!\n", __func__);
        return false;
    }

    return true;
}

bool ProposalFromTransaction(const CTransaction& tx, CVoteProposal& proposal)
{
    if (!tx.IsProposal())
        return error("%s : tx is not a proposal", __func__);

    vector<unsigned char> vchProposal;

    CScript scriptProposal = tx.vout[0].scriptPubKey;
    vchProposal.insert(vchProposal.end(), scriptProposal.begin() + 6, scriptProposal.end());
    CDataStream ss(vchProposal, SER_NETWORK, 0);

    try {
        ss >> proposal;
    } catch(std::exception& e) {
        return error("%s: failed to deserialize: %s ", __func__, e.what());
    }

    return true;
}

VoteLocation CVoteProposal::GetLocation() const
{
    VoteLocation location;
    location.second = static_cast<uint8_t>(GetShift() + 1);
    location.first = static_cast<uint8_t>(GetShift() + nBitCount);

    return location;
}