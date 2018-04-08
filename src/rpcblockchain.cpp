// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "bitcoinrpc.h"
#include "voteproposal.h"
#include "voteproposalmanager.h"
#include "voteobject.h"
#include "votetally.h"
#include "db.h"

#include <iostream>
#include <fstream>

using namespace json_spirit;
using namespace std;

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, json_spirit::Object& entry);

double GetDifficulty(const CBlockIndex* blockindex)
{
    // Floating point number that is a multiple of the minimum difficulty,
    // minimum difficulty = 1.0.
    if (blockindex == NULL)
    {
        if (pindexBest == NULL)
            return 1.0;
        else
            blockindex = GetLastBlockIndex(pindexBest, false);
    }

    int nShift = (blockindex->nBits >> 24) & 0xff;

    double dDiff =
        (double)0x0000ffff / (double)(blockindex->nBits & 0x00ffffff);

    while (nShift < 29)
    {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29)
    {
        dDiff /= 256.0;
        nShift--;
    }

    return dDiff;
}

double GetPoSKernelPS(const CBlockIndex* blockindex)
{
    int nPoSInterval = 72;
    double dStakeKernelsTriedAvg = 0;
    int nStakesHandled = 0, nStakesTime = 0;

    const CBlockIndex* pindex = pindexBest;
	const CBlockIndex* pindexPrevStake = NULL;

	if (blockindex != NULL)
		pindex = blockindex;
		
    while (pindex && nStakesHandled < nPoSInterval)
    {
        if (pindex->IsProofOfStake())
        {
            dStakeKernelsTriedAvg += GetDifficulty(pindex) * 4294967296.0;
            nStakesTime += pindexPrevStake ? (pindexPrevStake->nTime - pindex->nTime) : 0;
            pindexPrevStake = pindex;
            nStakesHandled++;
        }

        pindex = pindex->pprev;
    }

    return nStakesTime ? dStakeKernelsTriedAvg / nStakesTime : 0;
}

Object blockToJSON(const CBlock& block, const CBlockIndex* blockindex, bool fPrintTransactionDetail)
{
    Object result;
    result.push_back(Pair("hash", block.GetHash().GetHex()));
    CMerkleTx txGen(block.vtx[0]);
    txGen.SetMerkleBranch(&block);
    result.push_back(Pair("confirmations", (int)txGen.GetDepthInMainChain()));
    result.push_back(Pair("size", (int)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION)));
    result.push_back(Pair("height", blockindex->nHeight));
    result.push_back(Pair("version", block.nVersion));
    result.push_back(Pair("merkleroot", block.hashMerkleRoot.GetHex()));
    result.push_back(Pair("mint", ValueFromAmount(blockindex->nMint)));
    result.push_back(Pair("time", (boost::int64_t)block.GetBlockTime()));
    result.push_back(Pair("nonce", (boost::uint64_t)block.nNonce));
    result.push_back(Pair("bits", HexBits(block.nBits)));
    result.push_back(Pair("difficulty", GetDifficulty(blockindex)));

    if (blockindex->pprev)
        result.push_back(Pair("previousblockhash", blockindex->pprev->GetBlockHash().GetHex()));
    if (blockindex->pnext)
        result.push_back(Pair("nextblockhash", blockindex->pnext->GetBlockHash().GetHex()));

    result.push_back(Pair("flags", strprintf("%s%s", blockindex->IsProofOfStake()? "proof-of-stake" : "proof-of-work", blockindex->GeneratedStakeModifier()? " stake-modifier": "")));
    result.push_back(Pair("proofhash", blockindex->IsProofOfStake()? blockindex->hashProofOfStake.GetHex() : blockindex->GetBlockHash().GetHex()));
    result.push_back(Pair("entropybit", (int)blockindex->GetStakeEntropyBit()));
    result.push_back(Pair("modifier", strprintf("%016llx", blockindex->nStakeModifier)));
    result.push_back(Pair("modifierchecksum", strprintf("%08x", blockindex->nStakeModifierChecksum)));
    Array txinfo;
    BOOST_FOREACH (const CTransaction& tx, block.vtx)
    {
        if (fPrintTransactionDetail)
        {
            Object entry;

            entry.push_back(Pair("txid", tx.GetHash().GetHex()));
            TxToJSON(tx, 0, entry);

            txinfo.push_back(entry);
        }
        else
            txinfo.push_back(tx.GetHash().GetHex());
    }

    result.push_back(Pair("tx", txinfo));
    result.push_back(Pair("signature", HexStr(block.vchBlockSig.begin(), block.vchBlockSig.end())));

    return result;
}


Value getblockcount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getblockcount\n"
            "Returns the number of blocks in the longest block chain.");

    return nBestHeight;
}

//comment
Value getdifficulty(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getdifficulty\n"
            "Returns the difficulty as a multiple of the minimum difficulty.");

    Object obj;
    obj.push_back(Pair("proof-of-work",        GetDifficulty()));
    obj.push_back(Pair("proof-of-stake",       GetDifficulty(GetLastBlockIndex(pindexBest, true))));
    obj.push_back(Pair("search-interval",      (int)nLastCoinStakeSearchInterval));
    return obj;
}


Value settxfee(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 1 || AmountFromValue(params[0]) < MIN_TX_FEE)
        throw runtime_error(
            "settxfee <amount>\n"
            "<amount> is a real and is rounded to the nearest 0.01");

    nTransactionFee = AmountFromValue(params[0]);
    nTransactionFee = (nTransactionFee / CENT) * CENT;  // round to cent

    return true;
}

Value getrawmempool(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getrawmempool\n"
            "Returns all transaction ids in memory pool.");

    vector<uint256> vtxid;
    mempool.queryHashes(vtxid);

    Array a;
    BOOST_FOREACH(const uint256& hash, vtxid)
        a.push_back(hash.ToString());

    return a;
}

Value getblockhash(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getblockhash <index>\n"
            "Returns hash of block in best-block-chain at <index>.");

    int nHeight = params[0].get_int();
    if (nHeight < 0 || nHeight > nBestHeight)
        throw runtime_error("Block number out of range.");

    CBlockIndex* pblockindex = FindBlockByHeight(nHeight);
    return pblockindex->phashBlock->GetHex();
}

Value getblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getblock <hash> [txinfo]\n"
            "txinfo optional to print more detailed tx info\n"
            "Returns details of a block with given block-hash.");

    std::string strHash = params[0].get_str();
    uint256 hash(strHash);

    if (mapBlockIndex.count(hash) == 0)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hash];
    block.ReadFromDisk(pblockindex, true);

    return blockToJSON(block, pblockindex, params.size() > 1 ? params[1].get_bool() : false);
}

Value getblockbynumber(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getblock <number> [txinfo]\n"
            "txinfo optional to print more detailed tx info\n"
            "Returns details of a block with given block-number.");

    int nHeight = params[0].get_int();
    if (nHeight < 0 || nHeight > nBestHeight)
        throw runtime_error("Block number out of range.");

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hashBestChain];
    while (pblockindex->nHeight > nHeight)
        pblockindex = pblockindex->pprev;

    uint256 hash = *pblockindex->phashBlock;

    pblockindex = mapBlockIndex[hash];
    block.ReadFromDisk(pblockindex, true);

    return blockToJSON(block, pblockindex, params.size() > 1 ? params[1].get_bool() : false);
}

// presstab HyperStake
Value exportdifficulty(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "export difficulty <interval> <directory>\n"
            "interval will give difficulty for every Xth block\n"
            "directory is the location to export the csv: C:\\example.csv)");
	
	int nInterval = params[0].get_int();		
	std::string strDir = params[1].get_str();
    
	ofstream File; 
	
	File.open(strDir.c_str()); 
	File << "Block, Difficulty" << endl;
	for(int i = 0; i < nBestHeight; i += nInterval)
	{
		File << i;
		File << ",";		
		File << GetDifficulty(FindBlockByHeight(i)) << endl; 
	}
	File.close(); 
    return "succesfully exported";
}

// presstab HyperStake
Value listblocks(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "<listblocks> <highest height> <# of blocks to display>\n"
            "list basic information about a range of blocks\n");
    unsigned int nTopBlock = params[0].get_int();
    unsigned int nRange = params[1].get_int();
    Array arrRet;

    for(unsigned int i = 0; i < nRange; i++) 
    {
        Object blk;
        unsigned int nBlockNumber = nTopBlock - i;
        CBlockIndex* pindex = FindBlockByHeight(nBlockNumber);
        blk.push_back(Pair("height", pindex->nHeight));
        blk.push_back(Pair("hash", pindex->GetBlockHash().GetHex()));
        blk.push_back(Pair("time", (boost::int64_t)pindex->GetBlockTime()));
        blk.push_back(Pair("difficulty", GetDifficulty(pindex)));
        if(pindex->IsProofOfStake())
            blk.push_back(Pair("type", "PoS"));
        else
            blk.push_back(Pair("type", "PoW"));
        blk.push_back(Pair("minted", ValueFromAmount(pindex->nMint)));
        blk.push_back(Pair("money supply", ValueFromAmount(pindex->nMoneySupply)));

        arrRet.push_back(blk);
    }  

    return arrRet;
}

// tuningmind
Value createproposal(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 6)
        throw runtime_error(
                "createproposal \n<strName>\n<nShift>\n<nStartBlock>\n<nCheckSpan>\n<nBits>\n<strDescription>\n"
                "Returns new VoteProposal object with specified parameters\n");
    // name of issue
    string strName = params[0].get_str();
    // check version for existing proposals Shift
    uint8_t nShift = params[1].get_int();
    // start time - will be changed to int StartHeight. unix time stamp
    int64 nStartTime =  params[2].get_int();
    // number of blocks with votes to count
    int nCheckSpan = params[3].get_int();
    // cardinal items to vote on - convert to uint8 CheckSpan
    uint8_t nBits = params[4].get_int();
    // description of issue - will go in different tx
    std::string strDescription = params[5].get_str();

    // the bit location object of the proposal
    VoteLocation location(nShift + nBits - 1, nShift);

    Object results;
    CVoteProposal proposal(strName, nStartTime, nCheckSpan, strDescription, location);

    //! Add the constructed proposal to a partial transaction
    CTransaction tx;
    proposal.ConstructTransaction(tx);

    //! Add the partial transaction to our globally accessible proposals map so that it can be called from elsewhere
    uint256 hashProposal = tx.GetHash();
    mapPendingProposals.insert(make_pair(hashProposal, tx));

    results.emplace_back(Pair("proposal_hash", hashProposal.GetHex().c_str()));
    results.emplace_back(Pair("name", strName));
    results.emplace_back(Pair("shift", nShift));
    results.emplace_back(Pair("start_block", (boost::int64_t)nStartTime));
    results.emplace_back(Pair("check_span", nCheckSpan));
    results.emplace_back(Pair("bit_count", nBits));
    results.emplace_back(Pair("description", strDescription));

    return results;
}

Value listproposals(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getproposals\n"
            "list proposals that have been found on the blockchain\n");

    //! Grab each proposal by 1st getting the txid from mapProposals, 2nd using that txid to grab the proposal object from voteDB
    Array arrRet;
    CVoteDB voteDB("r");
    for (auto it : mapProposals) {
        CVoteProposal proposal;
        if (voteDB.ReadProposal(it.first, proposal)) {
            Object jsonProposal;
            jsonProposal.emplace_back(Pair("txhash", it.first.GetHex()));
            jsonProposal.emplace_back(Pair("proposalhash", proposal.GetHash().GetHex()));
            jsonProposal.emplace_back(Pair("name", proposal.GetName()));
            jsonProposal.emplace_back(Pair("description", proposal.GetDescription()));
            jsonProposal.emplace_back(Pair("shift", proposal.GetShift()));
            jsonProposal.emplace_back(Pair("start_block", (boost::int64_t)proposal.GetStartHeight()));
            jsonProposal.emplace_back(Pair("bit_count", proposal.GetBitCount()));
            arrRet.emplace_back(jsonProposal);
        }
    }

    return arrRet;
}

Value getproposalstatus(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
                "getproposalstatus\n"
                        "<txhash>\n"
                        "return the status of a proposal that is being voted on\n");

    uint256 hash(params[0].get_str());
    CVoteDB voteDB("r");
    CVoteProposal proposal;
    if (!voteDB.ReadProposal(hash, proposal))
        throw JSONRPCError(RPC_INVALID_PARAMETER, "failed to find proposal");

    if (nBestHeight < proposal.GetStartHeight()) {
        Object obj;
        obj.emplace_back(Pair("block_start", (int64_t)proposal.GetStartHeight()));
        return obj;
    }

    int nHeightEnd = proposal.GetStartHeight() + proposal.GetCheckSpan();
    int nHeightTally;
    if (nHeightEnd < nBestHeight)
        nHeightTally = nHeightEnd;
    else
        nHeightTally = nBestHeight;

    CVoteTally tally = FindBlockByHeight(nHeightTally)->tally;
    CVoteSummary summary;
    if (!tally.GetSummary(proposal.GetHash(), summary))
        throw JSONRPCError(RPC_DATABASE_ERROR, "failed to find proposal in vote tally");

    Object objRet;
    int64_t nBlocksVoted = nHeightTally - summary.nBlockStart;
    objRet.emplace_back(Pair("block_start", (int64_t)summary.nBlockStart));
    objRet.emplace_back(Pair("blocks_remaining", (int64_t)std::max(nHeightEnd - nBestHeight, 0)));
    objRet.emplace_back(Pair("total_blocks_voted", nBlocksVoted));
    objRet.emplace_back(Pair("yes_votes", (int64_t)summary.nYesTally));
    objRet.emplace_back(Pair("no_votes", (int64_t)summary.nNoTally));
    double nAbstain = nBlocksVoted - summary.nNoTally - summary.nYesTally;
    objRet.emplace_back(Pair("abstain_votes", nAbstain));
    double nRatio = 0;
    if (nBlocksVoted)
        nRatio = (double)summary.nYesTally / ((double)nBlocksVoted - nAbstain);

    objRet.emplace_back(Pair("ratio", nRatio));
    return objRet;
}

