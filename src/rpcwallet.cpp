// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet.h"
#include "walletdb.h"
#include "bitcoinrpc.h"
#include "init.h"
#include "main.h"
#include "base58.h"
#include "coincontrol.h"
#include "voteobject.h"

#include <sstream>
#include <boost/lexical_cast.hpp>

using namespace json_spirit;
using namespace std;
using namespace boost;

int64 nWalletUnlockTime;
CCoinControl* coinControl = new CCoinControl;
static CCriticalSection cs_nWalletUnlockTime;

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, json_spirit::Object& entry);
extern int64 nLastCoinStakeSearchInterval;
extern bool fGenerateBitcoins;

std::string HelpRequiringPassphrase()
{
    return pwalletMain->IsCrypted()
        ? "\nrequires wallet passphrase to be set with walletpassphrase first"
        : "";
}

void EnsureWalletIsUnlocked()
{
    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");
    if (pwalletMain->fWalletUnlockMintOnly)
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Wallet unlocked for block minting only.");
}

void WalletTxToJSON(const CWalletTx& wtx, Object& entry)
{
    int confirms = wtx.GetDepthInMainChain();
    entry.push_back(Pair("confirmations", confirms));
    if (wtx.IsCoinBase() || wtx.IsCoinStake())
        entry.push_back(Pair("generated", true));
    if (confirms)
    {
        entry.push_back(Pair("blockhash", wtx.hashBlock.GetHex()));
        entry.push_back(Pair("blockindex", wtx.nIndex));
        entry.push_back(Pair("blocktime", (boost::int64_t)(mapBlockIndex[wtx.hashBlock]->nTime)));
    }
    entry.push_back(Pair("txid", wtx.GetHash().GetHex()));
    entry.push_back(Pair("time", (boost::int64_t)wtx.GetTxTime()));
    entry.push_back(Pair("timereceived", (boost::int64_t)wtx.nTimeReceived));
    BOOST_FOREACH(const PAIRTYPE(string,string)& item, wtx.mapValue)
        entry.push_back(Pair(item.first, item.second));
}

string AccountFromValue(const Value& value)
{
    string strAccount = value.get_str();
    if (strAccount == "*")
        throw JSONRPCError(RPC_WALLET_INVALID_ACCOUNT_NAME, "Invalid account name");
    return strAccount;
}

Value getinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getinfo\n"
            "Returns an object containing various state info.");

    proxyType proxy;
    GetProxy(NET_IPV4, proxy);

    Object obj;
    obj.push_back(Pair("version",       FormatFullVersion()));
    obj.push_back(Pair("protocolversion",(int)PROTOCOL_VERSION));
    obj.push_back(Pair("walletversion", pwalletMain->GetVersion()));
    obj.push_back(Pair("balance",       ValueFromAmount(pwalletMain->GetBalance())));
    obj.push_back(Pair("newmint",       ValueFromAmount(pwalletMain->GetNewMint())));
    obj.push_back(Pair("stake",         ValueFromAmount(pwalletMain->GetStake())));
    obj.push_back(Pair("blocks",        (int)nBestHeight));
    obj.push_back(Pair("moneysupply",   ValueFromAmount(pindexBest->nMoneySupply)));
    obj.push_back(Pair("connections",   (int)vNodes.size()));
    obj.push_back(Pair("proxy",         (proxy.first.IsValid() ? proxy.first.ToStringIPPort() : string())));
    obj.push_back(Pair("ip",            addrSeenByPeer.ToStringIP()));
    obj.push_back(Pair("difficulty",    GetDifficulty(GetLastBlockIndex(pindexBest, true))));
    obj.push_back(Pair("testnet",       fTestNet));
    obj.push_back(Pair("keypoololdest", (boost::int64_t)pwalletMain->GetOldestKeyPoolTime()));
    obj.push_back(Pair("keypoolsize",   pwalletMain->GetKeyPoolSize()));
    obj.push_back(Pair("paytxfee",      ValueFromAmount(nTransactionFee)));
	obj.push_back(Pair("staking status", (fWalletStaking ? "Staking Active" : "Staking Not Active")));
	
	std::string strLockState = "";
	if(pwalletMain->IsLocked())
		strLockState = "Wallet Locked";
	else if(pwalletMain->fWalletUnlockMintOnly)
		strLockState = "Wallet Unlocked for Minting Only";
	else
		strLockState = "Wallet is Unlocked";
	obj.push_back(Pair("lock state", strLockState));
    if (pwalletMain->IsCrypted())
        obj.push_back(Pair("unlocked_until", (boost::int64_t)nWalletUnlockTime / 1000));
    obj.push_back(Pair("errors",        GetWarnings("statusbar")));
    return obj;
}

Value getnewpubkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getnewpubkey [account]\n"
            "Returns new public key for coinbase generation.");

    // Parse the account first so we don't generate a key if there's an error
    string strAccount;
    if (params.size() > 0)
        strAccount = AccountFromValue(params[0]);

    if (!pwalletMain->IsLocked())
        pwalletMain->TopUpKeyPool();

    // Generate a new key that is added to wallet
    CPubKey newKey;
    if (!pwalletMain->GetKeyFromPool(newKey, false))
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");
    CKeyID keyID = newKey.GetID();

    pwalletMain->SetAddressBookName(keyID, strAccount);
    vector<unsigned char> vchPubKey = newKey.Raw();

    return HexStr(vchPubKey.begin(), vchPubKey.end());
}

Value getnewaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getnewaddress [account]\n"
            "Returns a new HyperStake address for receiving payments.  "
            "If [account] is specified (recommended), it is added to the address book "
            "so payments received with the address will be credited to [account].");

    // Parse the account first so we don't generate a key if there's an error
    string strAccount;
    if (params.size() > 0)
        strAccount = AccountFromValue(params[0]);

    if (!pwalletMain->IsLocked())
        pwalletMain->TopUpKeyPool();

    // Generate a new key that is added to wallet
    CPubKey newKey;
    if (!pwalletMain->GetKeyFromPool(newKey, false))
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");
    CKeyID keyID = newKey.GetID();

    pwalletMain->SetAddressBookName(keyID, strAccount);

    return CBitcoinAddress(keyID).ToString();
}

CBitcoinAddress GetAccountAddress(string strAccount, bool bForceNew=false)
{
    CWalletDB walletdb(pwalletMain->strWalletFile);

    CAccount account;
    walletdb.ReadAccount(strAccount, account);

    bool bKeyUsed = false;

    // Check if the current key has been used
    if (account.vchPubKey.IsValid())
    {
        CScript scriptPubKey;
        scriptPubKey.SetDestination(account.vchPubKey.GetID());
        for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin();
             it != pwalletMain->mapWallet.end() && account.vchPubKey.IsValid();
             ++it)
        {
            const CWalletTx& wtx = (*it).second;
            BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                if (txout.scriptPubKey == scriptPubKey)
                    bKeyUsed = true;
        }
    }

    // Generate a new key
    if (!account.vchPubKey.IsValid() || bForceNew || bKeyUsed)
    {
        if (!pwalletMain->GetKeyFromPool(account.vchPubKey, false))
            throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");

        pwalletMain->SetAddressBookName(account.vchPubKey.GetID(), strAccount);
        walletdb.WriteAccount(strAccount, account);
    }

    return CBitcoinAddress(account.vchPubKey.GetID());
}

Value getaccountaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getaccountaddress <account>\n"
            "Returns the current HyperStake address for receiving payments to this account.");

    // Parse the account first so we don't generate a key if there's an error
    string strAccount = AccountFromValue(params[0]);

    Value ret;

    ret = GetAccountAddress(strAccount).ToString();

    return ret;
}

Value setaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "setaccount <HyperStakeaddress> <account>\n"
            "Sets the account associated with the given address.");

    CBitcoinAddress address(params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid HyperStake address");


    string strAccount;
    if (params.size() > 1)
        strAccount = AccountFromValue(params[1]);

    // Detect when changing the account of an address that is the 'unused current key' of another account:
    if (pwalletMain->mapAddressBook.count(address.Get()))
    {
        string strOldAccount = pwalletMain->mapAddressBook[address.Get()];
        if (address == GetAccountAddress(strOldAccount))
            GetAccountAddress(strOldAccount, true);
    }

    pwalletMain->SetAddressBookName(address.Get(), strAccount);

    return Value::null;
}

Value getaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getaccount <HyperStakeaddress>\n"
            "Returns the account associated with the given address.");

    CBitcoinAddress address(params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid HyperStake address");

    string strAccount;
    map<CTxDestination, string>::iterator mi = pwalletMain->mapAddressBook.find(address.Get());
    if (mi != pwalletMain->mapAddressBook.end() && !(*mi).second.empty())
        strAccount = (*mi).second;
    return strAccount;
}

Value getaddressesbyaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getaddressesbyaccount <account>\n"
            "Returns the list of addresses for the given account.");

    string strAccount = AccountFromValue(params[0]);

    // Find all addresses that have the given account
    Array ret;
    BOOST_FOREACH(const PAIRTYPE(CBitcoinAddress, string)& item, pwalletMain->mapAddressBook)
    {
        const CBitcoinAddress& address = item.first;
        const string& strName = item.second;
        if (strName == strAccount)
            ret.push_back(address.ToString());
    }
    return ret;
}

Value sendtoaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 4)
        throw runtime_error(
		"sendtoaddress <HyperStakeaddress> <amount> [comment] [comment-to]\n"
            "<amount> is a real and is rounded to the nearest 0.000001"
            + HelpRequiringPassphrase());

    CBitcoinAddress address(params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid HyperStake address");

    // Amount
    int64 nAmount = AmountFromValue(params[1]);

    if (nAmount < MIN_TXOUT_AMOUNT)
        throw JSONRPCError(-101, "Send amount too small");

    // Wallet comments
    CWalletTx wtx;
    if (params.size() > 2 && params[2].type() != null_type && !params[2].get_str().empty())
        wtx.mapValue["comment"] = params[2].get_str();
    if (params.size() > 3 && params[3].type() != null_type && !params[3].get_str().empty())
        wtx.mapValue["to"]      = params[3].get_str();

    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");

    string strError = pwalletMain->SendMoneyToDestination(address.Get(), nAmount, wtx, false, false);
    if (strError != "")
        throw JSONRPCError(RPC_WALLET_ERROR, strError);

    return wtx.GetHash().GetHex();
}

Value listaddressgroupings(const Array& params, bool fHelp)
{
    if (fHelp)
        throw runtime_error(
            "listaddressgroupings\n"
            "Lists groups of addresses which have had their common ownership\n"
            "made public by common use as inputs or as the resulting change\n"
            "in past transactions");

    Array jsonGroupings;
    map<CTxDestination, int64> balances = pwalletMain->GetAddressBalances();
    BOOST_FOREACH(set<CTxDestination> grouping, pwalletMain->GetAddressGroupings())
    {
        Array jsonGrouping;
        BOOST_FOREACH(CTxDestination address, grouping)
        {
            Array addressInfo;
            addressInfo.push_back(CBitcoinAddress(address).ToString());
            addressInfo.push_back(ValueFromAmount(balances[address]));
            {
                LOCK(pwalletMain->cs_wallet);
                if (pwalletMain->mapAddressBook.find(CBitcoinAddress(address).Get()) != pwalletMain->mapAddressBook.end())
                    addressInfo.push_back(pwalletMain->mapAddressBook.find(CBitcoinAddress(address).Get())->second);
            }
            jsonGrouping.push_back(addressInfo);
        }
        jsonGroupings.push_back(jsonGrouping);
    }
    return jsonGroupings;
}

Value signmessage(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "signmessage <HyperStakeaddress> <message>\n"
            "Sign a message with the private key of an address");

    EnsureWalletIsUnlocked();

    string strAddress = params[0].get_str();
    string strMessage = params[1].get_str();

    CBitcoinAddress addr(strAddress);
    if (!addr.IsValid())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");

    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key");

    CKey key;
    if (!pwalletMain->GetKey(keyID, key))
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key not available");

    CDataStream ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    vector<unsigned char> vchSig;
    if (!key.SignCompact(Hash(ss.begin(), ss.end()), vchSig))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sign failed");

    return EncodeBase64(&vchSig[0], vchSig.size());
}

Value verifymessage(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
            "verifymessage <HyperStakeaddress> <signature> <message>\n"
            "Verify a signed message");

    string strAddress  = params[0].get_str();
    string strSign     = params[1].get_str();
    string strMessage  = params[2].get_str();

    CBitcoinAddress addr(strAddress);
    if (!addr.IsValid())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");

    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key");

    bool fInvalid = false;
    vector<unsigned char> vchSig = DecodeBase64(strSign.c_str(), &fInvalid);

    if (fInvalid)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Malformed base64 encoding");

    CDataStream ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    CKey key;
    if (!key.SetCompactSignature(Hash(ss.begin(), ss.end()), vchSig))
        return false;

    return (key.GetPubKey().GetID() == keyID);
}

Value getreceivedbyaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getreceivedbyaddress <HyperStakeaddress> [minconf=1]\n"
            "Returns the total amount received by <HyperStakeaddress> in transactions with at least [minconf] confirmations.");

    // Bitcoin address
    CBitcoinAddress address = CBitcoinAddress(params[0].get_str());
    CScript scriptPubKey;
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid HyperStake address");
    scriptPubKey.SetDestination(address.Get());
    if (!IsMine(*pwalletMain,scriptPubKey))
        return (double)0.0;

    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();

    // Tally
    int64 nAmount = 0;
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        if (wtx.IsCoinBase() || wtx.IsCoinStake() || !wtx.IsFinal())
            continue;

        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
            if (txout.scriptPubKey == scriptPubKey)
                if (wtx.GetDepthInMainChain() >= nMinDepth)
                    nAmount += txout.nValue;
    }

    return  ValueFromAmount(nAmount);
}

void GetAccountAddresses(string strAccount, set<CTxDestination>& setAddress)
{
    BOOST_FOREACH(const PAIRTYPE(CTxDestination, string)& item, pwalletMain->mapAddressBook)
    {
        const CTxDestination& address = item.first;
        const string& strName = item.second;
        if (strName == strAccount)
            setAddress.insert(address);
    }
}

Value getreceivedbyaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getreceivedbyaccount <account> [minconf=1]\n"
            "Returns the total amount received by addresses with <account> in transactions with at least [minconf] confirmations.");

    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();

    // Get the set of pub keys assigned to account
    string strAccount = AccountFromValue(params[0]);
    set<CTxDestination> setAddress;
    GetAccountAddresses(strAccount, setAddress);

    // Tally
    int64 nAmount = 0;
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        if (wtx.IsCoinBase() || wtx.IsCoinStake() || !wtx.IsFinal())
            continue;

        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
        {
            CTxDestination address;
            if (ExtractDestination(txout.scriptPubKey, address) && IsMine(*pwalletMain, address) && setAddress.count(address))
                if (wtx.GetDepthInMainChain() >= nMinDepth)
                    nAmount += txout.nValue;
        }
    }

    return (double)nAmount / (double)COIN;
}

int64 GetAccountBalance(CWalletDB& walletdb, const string& strAccount, int nMinDepth)
{
    int64 nBalance = 0;

    // Tally wallet transactions
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        if (!wtx.IsFinal())
            continue;

        int64 nGeneratedImmature, nGeneratedMature, nReceived, nSent, nFee;
        wtx.GetAccountAmounts(strAccount, nGeneratedImmature, nGeneratedMature, nReceived, nSent, nFee);

        if (nReceived != 0 && wtx.GetDepthInMainChain() >= nMinDepth)
            nBalance += nReceived;

		if((wtx.IsCoinBaseOrStake() && wtx.GetDepthInMainChain() >= nMinDepth && wtx.GetBlocksToMaturity() == 0)
			|| !wtx.IsCoinBaseOrStake())
		{
			nBalance += nGeneratedMature - nSent - nFee;
		}
    }

    // Tally internal accounting entries
    nBalance += walletdb.GetAccountCreditDebit(strAccount);

    return nBalance;
}

int64 GetAccountBalance(const string& strAccount, int nMinDepth)
{
    CWalletDB walletdb(pwalletMain->strWalletFile);
    return GetAccountBalance(walletdb, strAccount, nMinDepth);
}

Value getbalance(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "getbalance [account] [minconf=1]\n"
            "If [account] is not specified, returns the server's total available balance.\n"
            "If [account] is specified, returns the balance in the account.");

    if (params.size() == 0)
        return  ValueFromAmount(pwalletMain->GetBalance());

    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();

    if (params[0].get_str() == "*") {
        // Calculate total balance a different way from GetBalance()
        // (GetBalance() sums up all unspent TxOuts)
        // getbalance and getbalance '*' should always return the same number.
        int64 nBalance = 0;
        for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
        {
            const CWalletTx& wtx = (*it).second;
            if (!wtx.IsFinal())
                continue;

            int64 allGeneratedImmature, allGeneratedMature, allFee;
            allGeneratedImmature = allGeneratedMature = allFee = 0;

            string strSentAccount;
            list<pair<CTxDestination, int64> > listReceived;
            list<pair<CTxDestination, int64> > listSent;
            wtx.GetAmounts(allGeneratedImmature, allGeneratedMature, listReceived, listSent, allFee, strSentAccount);
            if (wtx.GetDepthInMainChain() >= nMinDepth)
            {
                BOOST_FOREACH(const PAIRTYPE(CTxDestination,int64)& r, listReceived)
                    nBalance += r.second;
            }

			if((wtx.IsCoinBaseOrStake() && wtx.GetDepthInMainChain() >= nMinDepth && wtx.GetBlocksToMaturity() == 0)
				|| !wtx.IsCoinBaseOrStake())
			{
				BOOST_FOREACH(const PAIRTYPE(CTxDestination,int64)& r, listSent)
                nBalance -= r.second;
				nBalance -= allFee;
				nBalance += allGeneratedMature;
			}
        }
        return  ValueFromAmount(nBalance);
    }

    string strAccount = AccountFromValue(params[0]);

    int64 nBalance = GetAccountBalance(strAccount, nMinDepth);

    return ValueFromAmount(nBalance);
}

Value movecmd(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 5)
        throw runtime_error(
            "move <fromaccount> <toaccount> <amount> [minconf=1] [comment]\n"
            "Move from one account in your wallet to another.");

    string strFrom = AccountFromValue(params[0]);
    string strTo = AccountFromValue(params[1]);
    int64 nAmount = AmountFromValue(params[2]);

    if (nAmount < MIN_TXOUT_AMOUNT)
        throw JSONRPCError(-101, "Send amount too small");

    if (params.size() > 3)
        // unused parameter, used to be nMinDepth, keep type-checking it though
        (void)params[3].get_int();
    string strComment;
    if (params.size() > 4)
        strComment = params[4].get_str();

    CWalletDB walletdb(pwalletMain->strWalletFile);
    if (!walletdb.TxnBegin())
        throw JSONRPCError(RPC_DATABASE_ERROR, "database error");

	// check balance
	int64 nBalance = GetAccountBalance(strFrom, 1);
	
	// no moving balances less than or equal to 0
	if(nBalance <= 0  || nBalance <  nAmount)
		throw JSONRPCError(-101, "Not enough balance");
		
    int64 nNow = GetAdjustedTime();

    // Debit
    CAccountingEntry debit;
    debit.nOrderPos = pwalletMain->IncOrderPosNext(&walletdb);
    debit.strAccount = strFrom;
    debit.nCreditDebit = -nAmount;
    debit.nTime = nNow;
    debit.strOtherAccount = strTo;
    debit.strComment = strComment;
    walletdb.WriteAccountingEntry(debit);

    // Credit
    CAccountingEntry credit;
    credit.nOrderPos = pwalletMain->IncOrderPosNext(&walletdb);
    credit.strAccount = strTo;
    credit.nCreditDebit = nAmount;
    credit.nTime = nNow;
    credit.strOtherAccount = strFrom;
    credit.strComment = strComment;
    walletdb.WriteAccountingEntry(credit);

    if (!walletdb.TxnCommit())
        throw JSONRPCError(RPC_DATABASE_ERROR, "database error");

    return true;
}

Value sendfrom(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 6)
        throw runtime_error(
		"sendfrom <fromaccount> <toHyperStakeaddress> <amount> [minconf=1] [comment] [comment-to]\n"
            "<amount> is a real and is rounded to the nearest 0.000001"
            + HelpRequiringPassphrase());

    string strAccount = AccountFromValue(params[0]);
    CBitcoinAddress address(params[1].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid HyperStake address");
    int64 nAmount = AmountFromValue(params[2]);

    if (nAmount < MIN_TXOUT_AMOUNT)
        throw JSONRPCError(-101, "Send amount too small");

    int nMinDepth = 1;
    if (params.size() > 3)
        nMinDepth = params[3].get_int();

    CWalletTx wtx;
    wtx.strFromAccount = strAccount;
    if (params.size() > 4 && params[4].type() != null_type && !params[4].get_str().empty())
        wtx.mapValue["comment"] = params[4].get_str();
    if (params.size() > 5 && params[5].type() != null_type && !params[5].get_str().empty())
        wtx.mapValue["to"]      = params[5].get_str();

    EnsureWalletIsUnlocked();

    // Check funds
    int64 nBalance = GetAccountBalance(strAccount, nMinDepth);
    if (nAmount > nBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Account has insufficient funds");

    // Send
    string strError = pwalletMain->SendMoneyToDestination(address.Get(), nAmount, wtx, false, false);
    if (strError != "")
        throw JSONRPCError(RPC_WALLET_ERROR, strError);

    return wtx.GetHash().GetHex();
}

Value sendmany(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 4)
        throw runtime_error(
		"sendmany <fromaccount> {address:amount,...} [minconf=1] [comment]\n"
            "amounts are double-precision floating point numbers"
            + HelpRequiringPassphrase());

    string strAccount = AccountFromValue(params[0]);
    Object sendTo = params[1].get_obj();
    int nMinDepth = 1;
    if (params.size() > 2)
        nMinDepth = params[2].get_int();

    CWalletTx wtx;
	
    wtx.strFromAccount = strAccount;
    if (params.size() > 3 && params[3].type() != null_type && !params[3].get_str().empty())
        wtx.mapValue["comment"] = params[3].get_str();

    set<CBitcoinAddress> setAddress;
    vector<pair<CScript, int64> > vecSend;

    int64 totalAmount = 0;
    BOOST_FOREACH(const Pair& s, sendTo)
    {
        CBitcoinAddress address(s.name_);
        if (!address.IsValid())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid HyperStake address: ")+s.name_);

        if (setAddress.count(address))
            throw JSONRPCError(RPC_INVALID_PARAMETER, string("Invalid parameter, duplicated address: ")+s.name_);
        setAddress.insert(address);

        CScript scriptPubKey;
        scriptPubKey.SetDestination(address.Get());
        int64 nAmount = AmountFromValue(s.value_);

        if (nAmount < MIN_TXOUT_AMOUNT)
            throw JSONRPCError(-101, "Send amount too small");

        totalAmount += nAmount;

        vecSend.push_back(make_pair(scriptPubKey, nAmount));
    }

    EnsureWalletIsUnlocked();

    // Check funds
    int64 nBalance = GetAccountBalance(strAccount, nMinDepth);
    if (totalAmount > nBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Account has insufficient funds");

    // Send
    CReserveKey keyChange(pwalletMain);
    int64 nFeeRequired = 0;
    bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired, 1);
    if (!fCreated)
    {
        if (totalAmount + nFeeRequired > pwalletMain->GetBalance())
            throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction creation failed");
    }
    if (!pwalletMain->CommitTransaction(wtx, keyChange))
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction commit failed");

    return wtx.GetHash().GetHex();
}

Value addmultisigaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 3)
    {
        string msg = "addmultisigaddress <nrequired> <'[\"key\",\"key\"]'> [account]\n"
            "Add a nrequired-to-sign multisignature address to the wallet\"\n"
            "each key is a HyperStake address or hex-encoded public key\n"
            "If [account] is specified, assign address to [account].";
        throw runtime_error(msg);
    }

    int nRequired = params[0].get_int();
    const Array& keys = params[1].get_array();
    string strAccount;
    if (params.size() > 2)
        strAccount = AccountFromValue(params[2]);

    // Gather public keys
    if (nRequired < 1)
        throw runtime_error("a multisignature address must require at least one key to redeem");
    if ((int)keys.size() < nRequired)
        throw runtime_error(
            strprintf("not enough keys supplied "
                      "(got %lu keys, but need at least %d to redeem)", keys.size(), nRequired));
    std::vector<CKey> pubkeys;
    pubkeys.resize(keys.size());
    for (unsigned int i = 0; i < keys.size(); i++)
    {
        const std::string& ks = keys[i].get_str();

        // Case 1: Bitcoin address and we have full public key:
        CBitcoinAddress address(ks);
        if (address.IsValid())
        {
            CKeyID keyID;
            if (!address.GetKeyID(keyID))
                throw runtime_error(
                    strprintf("%s does not refer to a key",ks.c_str()));
            CPubKey vchPubKey;
            if (!pwalletMain->GetPubKey(keyID, vchPubKey))
                throw runtime_error(
                    strprintf("no full public key for address %s",ks.c_str()));
            if (!vchPubKey.IsValid() || !pubkeys[i].SetPubKey(vchPubKey))
                throw runtime_error(" Invalid public key: "+ks);
        }

        // Case 2: hex public key
        else if (IsHex(ks))
        {
            CPubKey vchPubKey(ParseHex(ks));
            if (!vchPubKey.IsValid() || !pubkeys[i].SetPubKey(vchPubKey))
                throw runtime_error(" Invalid public key: "+ks);
        }
        else
        {
            throw runtime_error(" Invalid public key: "+ks);
        }
    }

    // Construct using pay-to-script-hash:
    CScript inner;
    inner.SetMultisig(nRequired, pubkeys);
    CScriptID innerID = inner.GetID();
    pwalletMain->AddCScript(inner);

    pwalletMain->SetAddressBookName(innerID, strAccount);
    return CBitcoinAddress(innerID).ToString();
}


struct tallyitem
{
    int64 nAmount;
    int nConf;
    tallyitem()
    {
        nAmount = 0;
        nConf = std::numeric_limits<int>::max();
    }
};

Value ListReceived(const Array& params, bool fByAccounts)
{
    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 0)
        nMinDepth = params[0].get_int();

    // Whether to include empty accounts
    bool fIncludeEmpty = false;
    if (params.size() > 1)
        fIncludeEmpty = params[1].get_bool();

    // Tally
    map<CBitcoinAddress, tallyitem> mapTally;
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;

        if (wtx.IsCoinBase() || wtx.IsCoinStake() || !wtx.IsFinal())
            continue;

        int nDepth = wtx.GetDepthInMainChain();
        if (nDepth < nMinDepth)
            continue;

        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
        {
            CTxDestination address;
            if (!ExtractDestination(txout.scriptPubKey, address) || !IsMine(*pwalletMain, address))
                continue;

            tallyitem& item = mapTally[address];
            item.nAmount += txout.nValue;
            item.nConf = min(item.nConf, nDepth);
        }
    }

    // Reply
    Array ret;
    map<string, tallyitem> mapAccountTally;
    BOOST_FOREACH(const PAIRTYPE(CBitcoinAddress, string)& item, pwalletMain->mapAddressBook)
    {
        const CBitcoinAddress& address = item.first;
        const string& strAccount = item.second;
        map<CBitcoinAddress, tallyitem>::iterator it = mapTally.find(address);
        if (it == mapTally.end() && !fIncludeEmpty)
            continue;

        int64 nAmount = 0;
        int nConf = std::numeric_limits<int>::max();
        if (it != mapTally.end())
        {
            nAmount = (*it).second.nAmount;
            nConf = (*it).second.nConf;
        }

        if (fByAccounts)
        {
            tallyitem& item = mapAccountTally[strAccount];
            item.nAmount += nAmount;
            item.nConf = min(item.nConf, nConf);
        }
        else
        {
            Object obj;
            obj.push_back(Pair("address",       address.ToString()));
            obj.push_back(Pair("account",       strAccount));
            obj.push_back(Pair("amount",        ValueFromAmount(nAmount)));
            obj.push_back(Pair("confirmations", (nConf == std::numeric_limits<int>::max() ? 0 : nConf)));
            ret.push_back(obj);
        }
    }

    if (fByAccounts)
    {
        for (map<string, tallyitem>::iterator it = mapAccountTally.begin(); it != mapAccountTally.end(); ++it)
        {
            int64 nAmount = (*it).second.nAmount;
            int nConf = (*it).second.nConf;
            Object obj;
            obj.push_back(Pair("account",       (*it).first));
            obj.push_back(Pair("amount",        ValueFromAmount(nAmount)));
            obj.push_back(Pair("confirmations", (nConf == std::numeric_limits<int>::max() ? 0 : nConf)));
            ret.push_back(obj);
        }
    }

    return ret;
}

Value listreceivedbyaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "listreceivedbyaddress [minconf=1] [includeempty=false]\n"
            "[minconf] is the minimum number of confirmations before payments are included.\n"
            "[includeempty] whether to include addresses that haven't received any payments.\n"
            "Returns an array of objects containing:\n"
            "  \"address\" : receiving address\n"
            "  \"account\" : the account of the receiving address\n"
            "  \"amount\" : total amount received by the address\n"
            "  \"confirmations\" : number of confirmations of the most recent transaction included");

    return ListReceived(params, false);
}

Value listreceivedbyaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "listreceivedbyaccount [minconf=1] [includeempty=false]\n"
            "[minconf] is the minimum number of confirmations before payments are included.\n"
            "[includeempty] whether to include accounts that haven't received any payments.\n"
            "Returns an array of objects containing:\n"
            "  \"account\" : the account of the receiving addresses\n"
            "  \"amount\" : total amount received by addresses with this account\n"
            "  \"confirmations\" : number of confirmations of the most recent transaction included");

    return ListReceived(params, true);
}

void ListTransactions(const CWalletTx& wtx, const string& strAccount, int nMinDepth, bool fLong, Array& ret)
{
    int64 nGeneratedImmature, nGeneratedMature, nFee;
    string strSentAccount;
    list<pair<CTxDestination, int64> > listReceived;
    list<pair<CTxDestination, int64> > listSent;

    wtx.GetAmounts(nGeneratedImmature, nGeneratedMature, listReceived, listSent, nFee, strSentAccount);

    bool fAllAccounts = (strAccount == string("*"));

    // Generated blocks assigned to account ""
    if ((nGeneratedMature+nGeneratedImmature) != 0 && (fAllAccounts || strAccount == ""))
    {
        Object entry;
        entry.push_back(Pair("account", string("")));
        if (nGeneratedImmature)
        {
            entry.push_back(Pair("category", wtx.GetDepthInMainChain() ? "immature" : "orphan"));
            entry.push_back(Pair("amount", ValueFromAmount(nGeneratedImmature)));
        }
        else
        {
            entry.push_back(Pair("category", "generate"));
            entry.push_back(Pair("amount", ValueFromAmount(nGeneratedMature)));
        }
        if (fLong)
            WalletTxToJSON(wtx, entry);
        ret.push_back(entry);
    }

    // Sent
    if ((!listSent.empty() || nFee != 0) && (fAllAccounts || strAccount == strSentAccount))
    {
        BOOST_FOREACH(const PAIRTYPE(CTxDestination, int64)& s, listSent)
        {
            Object entry;
            entry.push_back(Pair("account", strSentAccount));
            entry.push_back(Pair("address", CBitcoinAddress(s.first).ToString()));
            entry.push_back(Pair("category", "send"));
            entry.push_back(Pair("amount", ValueFromAmount(-s.second)));
            entry.push_back(Pair("fee", ValueFromAmount(-nFee)));
            if (fLong)
                WalletTxToJSON(wtx, entry);
            ret.push_back(entry);
        }
    }

    // Received
    if (listReceived.size() > 0 && wtx.GetDepthInMainChain() >= nMinDepth)
    {
        BOOST_FOREACH(const PAIRTYPE(CTxDestination, int64)& r, listReceived)
        {
            string account;
            if (pwalletMain->mapAddressBook.count(r.first))
                account = pwalletMain->mapAddressBook[r.first];
            if (fAllAccounts || (account == strAccount))
            {
                Object entry;
                entry.push_back(Pair("account", account));
                entry.push_back(Pair("address", CBitcoinAddress(r.first).ToString()));
                if (wtx.IsCoinBase())
                {
                    if (wtx.GetDepthInMainChain() < 1)
                        entry.push_back(Pair("category", "orphan"));
                    else if (wtx.GetBlocksToMaturity() > 0)
                        entry.push_back(Pair("category", "immature"));
                    else
                        entry.push_back(Pair("category", "generate"));
                }
                else
                    entry.push_back(Pair("category", "receive"));
                entry.push_back(Pair("amount", ValueFromAmount(r.second)));
                if (fLong)
                    WalletTxToJSON(wtx, entry);
                ret.push_back(entry);
            }
        }
    }
}

void AcentryToJSON(const CAccountingEntry& acentry, const string& strAccount, Array& ret)
{
    bool fAllAccounts = (strAccount == string("*"));

    if (fAllAccounts || acentry.strAccount == strAccount)
    {
        Object entry;
        entry.push_back(Pair("account", acentry.strAccount));
        entry.push_back(Pair("category", "move"));
        entry.push_back(Pair("time", (boost::int64_t)acentry.nTime));
        entry.push_back(Pair("amount", ValueFromAmount(acentry.nCreditDebit)));
        entry.push_back(Pair("otheraccount", acentry.strOtherAccount));
        entry.push_back(Pair("comment", acentry.strComment));
        ret.push_back(entry);
    }
}

Value listtransactions(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 3)
        throw runtime_error(
            "listtransactions [account] [count=10] [from=0]\n"
            "Returns up to [count] most recent transactions skipping the first [from] transactions for account [account].");

    string strAccount = "*";
    if (params.size() > 0)
        strAccount = params[0].get_str();
    int nCount = 10;
    if (params.size() > 1)
        nCount = params[1].get_int();
    int nFrom = 0;
    if (params.size() > 2)
        nFrom = params[2].get_int();

    if (nCount < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative count");
    if (nFrom < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative from");

    Array ret;

    std::list<CAccountingEntry> acentries;
    CWallet::TxItems txOrdered = pwalletMain->OrderedTxItems(acentries, strAccount);

    // iterate backwards until we have nCount items to return:
    for (CWallet::TxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it)
    {
        CWalletTx *const pwtx = (*it).second.first;
        if (pwtx != 0)
            ListTransactions(*pwtx, strAccount, 0, true, ret);
        CAccountingEntry *const pacentry = (*it).second.second;
        if (pacentry != 0)
            AcentryToJSON(*pacentry, strAccount, ret);

        if ((int)ret.size() >= (nCount+nFrom)) break;
    }
    // ret is newest to oldest

    if (nFrom > (int)ret.size())
        nFrom = ret.size();
    if ((nFrom + nCount) > (int)ret.size())
        nCount = ret.size() - nFrom;
    Array::iterator first = ret.begin();
    std::advance(first, nFrom);
    Array::iterator last = ret.begin();
    std::advance(last, nFrom+nCount);

    if (last != ret.end()) ret.erase(last, ret.end());
    if (first != ret.begin()) ret.erase(ret.begin(), first);

    std::reverse(ret.begin(), ret.end()); // Return oldest to newest

    return ret;
}

Value listaccounts(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "listaccounts [minconf=1]\n"
            "Returns Object that has account names as keys, account balances as values.");

    int nMinDepth = 1;
    if (params.size() > 0)
        nMinDepth = params[0].get_int();

    map<string, int64> mapAccountBalances;
    BOOST_FOREACH(const PAIRTYPE(CTxDestination, string)& entry, pwalletMain->mapAddressBook) {
        if (IsMine(*pwalletMain, entry.first)) // This address belongs to me
            mapAccountBalances[entry.second] = 0;
    }

    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;

		if(!wtx.IsFinal())
			continue;

        int64 nGeneratedImmature, nGeneratedMature, nFee;
        string strSentAccount;
        list<pair<CTxDestination, int64> > listReceived;
        list<pair<CTxDestination, int64> > listSent;
        wtx.GetAmounts(nGeneratedImmature, nGeneratedMature, listReceived, listSent, nFee, strSentAccount);
        if (wtx.GetDepthInMainChain() >= nMinDepth)
        {
            BOOST_FOREACH(const PAIRTYPE(CTxDestination, int64)& r, listReceived)
                if (pwalletMain->mapAddressBook.count(r.first))
                    mapAccountBalances[pwalletMain->mapAddressBook[r.first]] += r.second;
                else
                    mapAccountBalances[""] += r.second;
        }

		if((wtx.IsCoinBaseOrStake() && wtx.GetDepthInMainChain() >= nMinDepth && wtx.GetBlocksToMaturity() == 0)
			|| !wtx.IsCoinBaseOrStake())
		{
			mapAccountBalances[strSentAccount] -= nFee;
			mapAccountBalances[""] += nGeneratedMature;

			BOOST_FOREACH(const PAIRTYPE(CTxDestination, int64)& s, listSent)
				mapAccountBalances[strSentAccount] -= s.second;

		}
    }

    list<CAccountingEntry> acentries;
    CWalletDB(pwalletMain->strWalletFile).ListAccountCreditDebit("*", acentries);
    BOOST_FOREACH(const CAccountingEntry& entry, acentries)
        mapAccountBalances[entry.strAccount] += entry.nCreditDebit;

    Object ret;
    BOOST_FOREACH(const PAIRTYPE(string, int64)& accountBalance, mapAccountBalances) {
        ret.push_back(Pair(accountBalance.first, ValueFromAmount(accountBalance.second)));
    }
    return ret;
}

Value deleteaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "delete <address>\n"
            "Deletes an address from wallet.dat, use with caution. Cannot be restored.");
	
	
	string strAdd = params[0].get_str();
	
	CWalletDB(pwalletMain->strWalletFile).EraseName(strAdd);
	pwalletMain->TopUpKeyPool();
	
	string ret = "Success, please restart wallet if using QT";
	return ret;
}

Value listsinceblock(const Array& params, bool fHelp)
{
    if (fHelp)
        throw runtime_error(
            "listsinceblock [blockhash] [target-confirmations]\n"
            "Get all transactions in blocks since block [blockhash], or all transactions if omitted");

    CBlockIndex *pindex = NULL;
    int target_confirms = 1;

    if (params.size() > 0)
    {
        uint256 blockId = 0;

        blockId.SetHex(params[0].get_str());
        pindex = CBlockLocator(blockId).GetBlockIndex();
    }

    if (params.size() > 1)
    {
        target_confirms = params[1].get_int();

        if (target_confirms < 1)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter");
    }

    int depth = pindex ? (1 + nBestHeight - pindex->nHeight) : -1;

    Array transactions;

    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); it++)
    {
        CWalletTx tx = (*it).second;

        if (depth == -1 || tx.GetDepthInMainChain() < depth)
            ListTransactions(tx, "*", 0, true, transactions);
    }

    uint256 lastblock;

    if (target_confirms == 1)
    {
        lastblock = hashBestChain;
    }
    else
    {
        int target_height = pindexBest->nHeight + 1 - target_confirms;

        CBlockIndex *block;
        for (block = pindexBest;
             block && block->nHeight > target_height;
             block = block->pprev)  { }

        lastblock = block ? block->GetBlockHash() : 0;
    }

    Object ret;
    ret.push_back(Pair("transactions", transactions));
    ret.push_back(Pair("lastblock", lastblock.GetHex()));

    return ret;
}

Value getconfs(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getconfs <txid>\n"
            "returns the number of confirmations for <txid>");

    uint256 hash;
    hash.SetHex(params[0].get_str());
	
	Object entry;
	CTransaction tx;
    uint256 hashBlock = 0;
    if (GetTransaction(hash, tx, hashBlock))
    {
        if (hashBlock == 0)
			entry.push_back(Pair("confirmations", 0));
		else
		{
			map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
			if (mi != mapBlockIndex.end() && (*mi).second)
			{
				CBlockIndex* pindex = (*mi).second;
				if (pindex->IsInMainChain())
				{
					entry.push_back(Pair("confirmations", 1 + nBestHeight - pindex->nHeight));
				}
				else
					entry.push_back(Pair("confirmations", 0));
            }
        }
		return entry;
	}
	else return "failed";
}

Value gettransaction(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "gettransaction <txid>\n"
            "Get detailed information about <txid>");

    uint256 hash;
    hash.SetHex(params[0].get_str());

    Object entry;

    if (pwalletMain->mapWallet.count(hash))
    {
        const CWalletTx& wtx = pwalletMain->mapWallet[hash];

        TxToJSON(wtx, 0, entry);

        int64 nCredit = wtx.GetCredit();
        int64 nDebit = wtx.GetDebit();
        int64 nNet = nCredit - nDebit;
        int64 nFee = (wtx.IsFromMe() ? wtx.GetValueOut() - nDebit : 0);

        entry.push_back(Pair("amount", ValueFromAmount(nNet - nFee)));
        if (wtx.IsFromMe())
            entry.push_back(Pair("fee", ValueFromAmount(nFee)));

        WalletTxToJSON(wtx, entry);

        Array details;
        ListTransactions(pwalletMain->mapWallet[hash], "*", 0, false, details);
        entry.push_back(Pair("details", details));
    }
    else
    {
        CTransaction tx;
        uint256 hashBlock = 0;
        if (GetTransaction(hash, tx, hashBlock))
        {
            entry.push_back(Pair("txid", hash.GetHex()));
            TxToJSON(tx, 0, entry);
            if (hashBlock == 0)
                entry.push_back(Pair("confirmations", 0));
            else
            {
                entry.push_back(Pair("blockhash", hashBlock.GetHex()));
                map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
                if (mi != mapBlockIndex.end() && (*mi).second)
                {
                    CBlockIndex* pindex = (*mi).second;
                    if (pindex->IsInMainChain())
                    {
                        entry.push_back(Pair("confirmations", 1 + nBestHeight - pindex->nHeight));
                        entry.push_back(Pair("txntime", (boost::int64_t)tx.nTime));
                        entry.push_back(Pair("time", (boost::int64_t)pindex->nTime));
                    }
                    else
                        entry.push_back(Pair("confirmations", 0));
                }
            }
        }
        else
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available about transaction");
    }

    return entry;
}

Value backupwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "backupwallet <destination>\n"
            "Safely copies wallet.dat to destination, which can be a directory or a path with filename.");

    string strDest = params[0].get_str();
    if (!BackupWallet(*pwalletMain, strDest))
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: Wallet backup failed!");

    return Value::null;
}

Value keypoolrefill(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 0)
        throw runtime_error(
            "keypoolrefill\n"
            "Fills the keypool."
            + HelpRequiringPassphrase());

    EnsureWalletIsUnlocked();

    pwalletMain->TopUpKeyPool();

    if (pwalletMain->GetKeyPoolSize() < GetArg("-keypool", 100))
        throw JSONRPCError(RPC_WALLET_ERROR, "Error refreshing keypool.");

    return Value::null;
}

void ThreadTopUpKeyPool(void* parg)
{
    // Make this thread recognisable as the key-topping-up thread
    RenameThread("bitcoin-key-top");

    pwalletMain->TopUpKeyPool();
}

void ThreadCleanWalletPassphrase(void* parg)
{
    // Make this thread recognisable as the wallet relocking thread
    RenameThread("bitcoin-lock-wa");

    int64 nMyWakeTime = GetTimeMillis() + *((int64*)parg) * 1000;

    ENTER_CRITICAL_SECTION(cs_nWalletUnlockTime);

    if (nWalletUnlockTime == 0)
    {
        nWalletUnlockTime = nMyWakeTime;

        do
        {
            if (nWalletUnlockTime==0)
                break;
            int64 nToSleep = nWalletUnlockTime - GetTimeMillis();
            if (nToSleep <= 0)
                break;

            LEAVE_CRITICAL_SECTION(cs_nWalletUnlockTime);
            Sleep(nToSleep);
            ENTER_CRITICAL_SECTION(cs_nWalletUnlockTime);

        } while(1);

        if (nWalletUnlockTime)
        {
            nWalletUnlockTime = 0;
            pwalletMain->Lock();
        }
    }
    else
    {
        if (nWalletUnlockTime < nMyWakeTime)
            nWalletUnlockTime = nMyWakeTime;
    }

    LEAVE_CRITICAL_SECTION(cs_nWalletUnlockTime);

    delete (int64*)parg;
}

Value walletpassphrase(const Array& params, bool fHelp)
{
    if (pwalletMain->IsCrypted() && (fHelp || params.size() < 2 || params.size() > 3))
        throw runtime_error(
            "walletpassphrase <passphrase> <timeout> [mintonly]\n"
            "Stores the wallet decryption key in memory for <timeout> seconds.\n"
            "mintonly is optional true/false allowing only block minting.");
    if (fHelp)
        return true;
    if (!pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletpassphrase was called.");

    if (!pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_ALREADY_UNLOCKED, "Error: Wallet is already unlocked, use walletlock first if need to change unlock settings.");
    // Note that the walletpassphrase is stored in params[0] which is not mlock()ed
    SecureString strWalletPass;
    strWalletPass.reserve(100);
    // TODO: get rid of this .c_str() by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    strWalletPass = params[0].get_str().c_str();

    if (strWalletPass.length() > 0)
    {
        if (!pwalletMain->Unlock(strWalletPass))
            throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered was incorrect.");
    }
    else
        throw runtime_error(
            "walletpassphrase <passphrase> <timeout>\n"
            "Stores the wallet decryption key in memory for <timeout> seconds.");

    NewThread(ThreadTopUpKeyPool, NULL);
    int64* pnSleepTime = new int64(params[1].get_int64());
    NewThread(ThreadCleanWalletPassphrase, pnSleepTime);

    // ppcoin: if user OS account compromised prevent trivial sendmoney commands
    if (params.size() > 2)
        pwalletMain->fWalletUnlockMintOnly = params[2].get_bool();
    else
        pwalletMain->fWalletUnlockMintOnly = false;
		
    return Value::null;
}

Value walletpassphrasechange(const Array& params, bool fHelp)
{
    if (pwalletMain->IsCrypted() && (fHelp || params.size() != 2))
        throw runtime_error(
            "walletpassphrasechange <oldpassphrase> <newpassphrase>\n"
            "Changes the wallet passphrase from <oldpassphrase> to <newpassphrase>.");
    if (fHelp)
        return true;
    if (!pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletpassphrasechange was called.");

    // TODO: get rid of these .c_str() calls by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    SecureString strOldWalletPass;
    strOldWalletPass.reserve(100);
    strOldWalletPass = params[0].get_str().c_str();

    SecureString strNewWalletPass;
    strNewWalletPass.reserve(100);
    strNewWalletPass = params[1].get_str().c_str();

    if (strOldWalletPass.length() < 1 || strNewWalletPass.length() < 1)
        throw runtime_error(
            "walletpassphrasechange <oldpassphrase> <newpassphrase>\n"
            "Changes the wallet passphrase from <oldpassphrase> to <newpassphrase>.");

    if (!pwalletMain->ChangeWalletPassphrase(strOldWalletPass, strNewWalletPass))
        throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered was incorrect.");

    return Value::null;
}

Value walletlock(const Array& params, bool fHelp)
{
    if (pwalletMain->IsCrypted() && (fHelp || params.size() != 0))
        throw runtime_error(
            "walletlock\n"
            "Removes the wallet encryption key from memory, locking the wallet.\n"
            "After calling this method, you will need to call walletpassphrase again\n"
            "before being able to call any methods which require the wallet to be unlocked.");
    if (fHelp)
        return true;
    if (!pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletlock was called.");

    {
        LOCK(cs_nWalletUnlockTime);
        pwalletMain->Lock();
        nWalletUnlockTime = 0;
    }

    return Value::null;
}

Value encryptwallet(const Array& params, bool fHelp)
{
    if (!pwalletMain->IsCrypted() && (fHelp || params.size() != 1))
        throw runtime_error(
            "encryptwallet <passphrase>\n"
            "Encrypts the wallet with <passphrase>.");
    if (fHelp)
        return true;
    if (pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an encrypted wallet, but encryptwallet was called.");

    // TODO: get rid of this .c_str() by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    SecureString strWalletPass;
    strWalletPass.reserve(100);
    strWalletPass = params[0].get_str().c_str();

    if (strWalletPass.length() < 1)
        throw runtime_error(
            "encryptwallet <passphrase>\n"
            "Encrypts the wallet with <passphrase>.");

    if (!pwalletMain->EncryptWallet(strWalletPass))
        throw JSONRPCError(RPC_WALLET_ENCRYPTION_FAILED, "Error: Failed to encrypt the wallet.");

    // BDB seems to have a bad habit of writing old data into
    // slack space in .dat files; that is bad if the old data is
    // unencrypted private keys. So:
    StartShutdown();
    return "wallet encrypted; HyperStake server stopping, restart to run with encrypted wallet.  The keypool has been flushed, you need to make a new backup.";
}

class DescribeAddressVisitor : public boost::static_visitor<Object>
{
public:
    Object operator()(const CNoDestination &dest) const { return Object(); }

    Object operator()(const CKeyID &keyID) const {
        Object obj;
        CPubKey vchPubKey;
        pwalletMain->GetPubKey(keyID, vchPubKey);
        obj.push_back(Pair("isscript", false));
        obj.push_back(Pair("pubkey", HexStr(vchPubKey.Raw())));
        obj.push_back(Pair("iscompressed", vchPubKey.IsCompressed()));
        return obj;
    }

    Object operator()(const CScriptID &scriptID) const {
        Object obj;
        obj.push_back(Pair("isscript", true));
        CScript subscript;
        pwalletMain->GetCScript(scriptID, subscript);
        std::vector<CTxDestination> addresses;
        txnouttype whichType;
        int nRequired;
        ExtractDestinations(subscript, whichType, addresses, nRequired);
        obj.push_back(Pair("script", GetTxnOutputType(whichType)));
        Array a;
        BOOST_FOREACH(const CTxDestination& addr, addresses)
            a.push_back(CBitcoinAddress(addr).ToString());
        obj.push_back(Pair("addresses", a));
        if (whichType == TX_MULTISIG)
            obj.push_back(Pair("sigsrequired", nRequired));
        return obj;
    }
};

Value validateaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "validateaddress <HyperStakeaddress>\n"
            "Return information about <HyperStakeaddress>.");

    CBitcoinAddress address(params[0].get_str());
    bool isValid = address.IsValid();

    Object ret;
    ret.push_back(Pair("isvalid", isValid));
    if (isValid)
    {
        CTxDestination dest = address.Get();
        string currentAddress = address.ToString();
        ret.push_back(Pair("address", currentAddress));
        bool fMine = IsMine(*pwalletMain, dest);
        ret.push_back(Pair("ismine", fMine));
        if (fMine) {
            Object detail = boost::apply_visitor(DescribeAddressVisitor(), dest);
            ret.insert(ret.end(), detail.begin(), detail.end());
        }
        if (pwalletMain->mapAddressBook.count(dest))
            ret.push_back(Pair("account", pwalletMain->mapAddressBook[dest]));
    }
    return ret;
}

Value validatepubkey(const Array& params, bool fHelp)
{
    if (fHelp || !params.size() || params.size() > 2)
        throw runtime_error(
            "validatepubkey <HyperStakepubkey>\n"
            "Return information about <HyperStakepubkey>.");

    std::vector<unsigned char> vchPubKey = ParseHex(params[0].get_str());
    CPubKey pubKey(vchPubKey);

    bool isValid = pubKey.IsValid();
    bool isCompressed = pubKey.IsCompressed();
    CKeyID keyID = pubKey.GetID();

    CBitcoinAddress address;
    address.Set(keyID);

    Object ret;
    ret.push_back(Pair("isvalid", isValid));
    if (isValid)
    {
        CTxDestination dest = address.Get();
        string currentAddress = address.ToString();
        ret.push_back(Pair("address", currentAddress));
        bool fMine = IsMine(*pwalletMain, dest);
        ret.push_back(Pair("ismine", fMine));
        ret.push_back(Pair("iscompressed", isCompressed));
        if (fMine) {
            Object detail = boost::apply_visitor(DescribeAddressVisitor(), dest);
            ret.insert(ret.end(), detail.begin(), detail.end());
        }
        if (pwalletMain->mapAddressBook.count(dest))
            ret.push_back(Pair("account", pwalletMain->mapAddressBook[dest]));
    }
    return ret;
}

// ppcoin: reserve balance from being staked for network protection
Value reservebalance(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "reservebalance [<reserve> [amount]]\n"
            "<reserve> is true or false to turn balance reserve on or off.\n"
            "<amount> is a real and rounded to cent.\n"
            "Set reserve amount not participating in network protection.\n"
            "If no parameters provided current setting is printed.\n");

    if (params.size() > 0)
    {
        bool fReserve = params[0].get_bool();
        if (fReserve)
        {
            if (params.size() == 1)
                throw runtime_error("must provide amount to reserve balance.\n");
            int64 nAmount = AmountFromValue(params[1]);
            nAmount = (nAmount / CENT) * CENT;  // round to cent
            if (nAmount < 0)
                throw runtime_error("amount cannot be negative.\n");
            mapArgs["-reservebalance"] = FormatMoney(nAmount).c_str();
        }
        else
        {
            if (params.size() > 1)
                throw runtime_error("cannot specify amount to turn off reserve.\n");
            mapArgs["-reservebalance"] = "0";
        }
    }

    Object result;
    int64 nReserveBalance = 0;
    if (mapArgs.count("-reservebalance") && !ParseMoney(mapArgs["-reservebalance"], nReserveBalance))
        throw runtime_error("invalid reserve balance amount\n");
    result.push_back(Pair("reserve", (nReserveBalance > 0)));
    result.push_back(Pair("amount", ValueFromAmount(nReserveBalance)));
    return result;
}

// ppcoin: check wallet integrity
Value checkwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 0)
        throw runtime_error(
            "checkwallet\n"
            "Check wallet for integrity.\n");

    int nMismatchSpent;
    int64 nBalanceInQuestion;
    int nOrphansFound;
    pwalletMain->FixSpentCoins(nMismatchSpent, nBalanceInQuestion, nOrphansFound, true);
    Object result;
    if (nMismatchSpent == 0 && nOrphansFound == 0)
        result.push_back(Pair("wallet check passed", true));
    else
    {
        result.push_back(Pair("mismatched spent coins", nMismatchSpent));
        result.push_back(Pair("amount in question", ValueFromAmount(nBalanceInQuestion)));
        result.push_back(Pair("orphan blocks found", nOrphansFound));
    }
    return result;
}

// ppcoin: repair wallet
Value repairwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 0)
        throw runtime_error(
            "repairwallet\n"
            "Repair wallet if checkwallet reports any problem.\n");

    int nMismatchSpent;
    int64 nBalanceInQuestion;
    int nOrphansFound;
    pwalletMain->FixSpentCoins(nMismatchSpent, nBalanceInQuestion,nOrphansFound);
    Object result;
    if (nMismatchSpent == 0 && nOrphansFound == 0)
        result.push_back(Pair("wallet check passed", true));
    else
    {
        result.push_back(Pair("mismatched spent coins", nMismatchSpent));
        result.push_back(Pair("amount affected by repair", ValueFromAmount(nBalanceInQuestion)));
        result.push_back(Pair("orphan blocks removed", nOrphansFound));
    }
    return result;
}

// resend unconfirmed wallet transactions
Value resendtx(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "resendtx\n"
            "Re-send unconfirmed transactions.\n"
        );

    ResendWalletTransactions();

    return Value::null;
}

// ppcoin: make a public-private key pair
Value makekeypair(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "makekeypair [prefix]\n"
            "Make a public/private key pair.\n"
            "[prefix] is optional preferred prefix for the public key.\n");

    string strPrefix = "";
    if (params.size() > 0)
        strPrefix = params[0].get_str();
 
    CKey key;
    key.MakeNewKey(false);

    CPrivKey vchPrivKey = key.GetPrivKey();
    Object result;
    result.push_back(Pair("PrivateKey", HexStr<CPrivKey::iterator>(vchPrivKey.begin(), vchPrivKey.end())));
    result.push_back(Pair("PublicKey", HexStr(key.GetPubKey().Raw())));
    return result;
}

/** HyperStake Specific RPC Wallet Additions**/

//presstab
double GetMoneySupply(int nHeight)
{
	CBlockIndex* pindex = FindBlockByHeight(nHeight);
	double nSupply = pindex->nMoneySupply;	
	return nSupply / COIN;	
}
//presstab
double GetSupplyChange(int nHeight, int pHeight)
{
	double nSupply = GetMoneySupply(nHeight); //present supply
	double pSupply = GetMoneySupply(pHeight); //previous supply
	double nChange = nSupply - pSupply; //difference
	return nChange;
}

//presstab
double GetBlockSpeed(int nHeight, int pHeight)
{
	CBlockIndex* pIndex = FindBlockByHeight(nHeight);
	CBlockIndex* ppIndex = FindBlockByHeight(pHeight);
	double nTime = pIndex->nTime;
	double pTime = ppIndex->nTime;
	double nTimeChange = (nTime - pTime) / 60 / 60 / 24; //in days
	return nTimeChange;
}
//presstab
double GetRate(int nHeight, int pHeight)
{
	double nSupplyChange = GetSupplyChange(nHeight, pHeight);
	double nTimeChange = GetBlockSpeed(nHeight, pHeight);
	double nMoneySupply = GetMoneySupply(nHeight);
	double nRate = nSupplyChange / nMoneySupply / nTimeChange;
	
	return nRate;
}

//presstab
double PredictFutureSupply(int nHeight, int pHeight, int nDays)
{
	double nRate = GetRate(nHeight, pHeight);
	double nSupply = GetMoneySupply(nHeight);
	double fSupply = nSupply * pow( 1 + nRate, nDays ); //compounds daily
	
	return fSupply;
}

//presstab
Value getmoneysupply(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getmoneysupply [height]\n"
            "Returns money supply at certain block, current money supply as default");
	
	GetLastBlockIndex(pindexBest, false);

	int nHeight = 0;
	double nMoneySupply = 0;
	
    if (params.size() > 0)
	{
		nHeight = pindexBest->nHeight;
		int pHeight = params[0].get_int();
		if (pHeight > nHeight || pHeight < 0)
			nMoneySupply = 0;
		else
			nMoneySupply = GetMoneySupply(pHeight);
	}
	else
	{
		nHeight = pindexBest->nHeight;
		nMoneySupply = GetMoneySupply(nHeight);
	}	
	Object obj;
	obj.push_back(Pair("money supply", nMoneySupply));
    return obj;
}

//Presstab's Preferred Money Supply Information
Value moneysupply(const Array& params, bool fHelp)
{
	if (fHelp || params.size() != 0)
        throw runtime_error(
            "moneysupply\n"
            "Show important money supply variables.\n");
	
	// grab block index of last block
	GetLastBlockIndex(pindexBest, false);
	
	//height of blocks
	int64 nHeight = pindexBest->nHeight; //present
	int64 n1Height = nHeight - 960; // day -- 960 blocks should be about 1 day if blocks have 90 sec spacing
	int64 n7Height = nHeight - 960 * 7; // week
	int64 n30Height = nHeight - 960 * 30; // month
	int64 forkHeight = 31781;
	

	//print to console
	Object obj;
	obj.push_back(Pair("moneysupply - present", GetMoneySupply(nHeight)));
	obj.push_back(Pair("moneysupply - 960 blocks ago", GetMoneySupply(n1Height)));
	obj.push_back(Pair("moneysupply - 6,720 blocks ago", GetMoneySupply(n7Height)));
	obj.push_back(Pair("moneysupply - 28,800 blocks ago", GetMoneySupply(n30Height)));
	
	obj.push_back(Pair("supply change(last 960 blocks)", GetSupplyChange(nHeight, n1Height)));
	obj.push_back(Pair("supply change(last 6,720 blocks)", GetSupplyChange(nHeight, n7Height)));
	obj.push_back(Pair("supply change(last 28,800 blocks)", GetSupplyChange(nHeight, n30Height)));
	obj.push_back(Pair("supply change since fork (block 31781)", GetSupplyChange(nHeight, forkHeight - 1)));
	
	obj.push_back(Pair("time change over 960 blocks", GetBlockSpeed(nHeight, n1Height)));
	obj.push_back(Pair("time change over 6,720 blocks", GetBlockSpeed(nHeight, n7Height)));
	obj.push_back(Pair("time change over 28,800 blocks", GetBlockSpeed(nHeight, n30Height)));
	
	obj.push_back(Pair("avg daily rate of change (last 960 blocks)", GetRate(nHeight, n1Height)));
	obj.push_back(Pair("avg daily rate of change (last 6,720 blocks)", GetRate(nHeight, n7Height)));
	obj.push_back(Pair("avg daily rate of change (last 28,800 blocks)", GetRate(nHeight, n30Height)));
	
    obj.push_back(Pair("projected money supply 1 day from now (daily compound)", PredictFutureSupply(nHeight, n1Height, 1)));
    obj.push_back(Pair("projected money supply 7 days from now (daily compound)", PredictFutureSupply(nHeight, n7Height, 7)));
    obj.push_back(Pair("projected money supply 30  days from now (daily compound)", PredictFutureSupply(nHeight, n30Height, 30)));
	
    //obj.push_back(Pair("projected money supply 365 days from now (using avg of 3 rates)", ms0 * pow (1 + (r1 + r7 +r30) / 3 * 8, 365 / 8)));
	return obj;
}

//presstab HyperStake
Value getstaketx(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getstaketx <txid>\n"
            "Get detailed information about a specific stake <txid>");

    uint256 hash;
    hash.SetHex(params[0].get_str());

    Object entry;
	Array vin;

    if (pwalletMain->mapWallet.count(hash))
    {
        const CWalletTx& wtx = pwalletMain->mapWallet[hash];
		
		 BOOST_FOREACH(const CTxIn& txin, wtx.vin)
		{
			Object in;
			if (wtx.IsCoinBase())
				entry.push_back(Pair("coinbase", HexStr(txin.scriptSig.begin(), txin.scriptSig.end())));
			else
			{
				CTransaction& txPrev = pwalletMain->mapWallet[txin.prevout.hash]; //first transaction
				uint64_t nTime = wtx.nTime; //stake tx time
				uint64_t nPrevTime = txPrev.nTime; //previous tx time
				uint64_t nTimeToStake = nTime - nPrevTime; // time to stake in seconds
				double dDaysToStake = nTimeToStake / 60.00 / 60 / 24;
				
				int64 nDebit = wtx.GetDebit();
				int64 nFee = (wtx.IsFromMe() ? wtx.GetValueOut() - nDebit : 0);

				int64 nGeneratedImmature, nGeneratedMature, nFee2;
				string strSentAccount;
				list<pair<CTxDestination, int64> > listReceived;
				list<pair<CTxDestination, int64> > listSent;
				wtx.GetAmounts(nGeneratedImmature, nGeneratedMature, listReceived, listSent, nFee2, strSentAccount);
				uint64_t nGeneratedAmount = max (nGeneratedMature, nGeneratedImmature);
				double nGeneratedAmount2 = max (nGeneratedMature, nGeneratedImmature); //uint64_t math not working
				double percentReward = nFee / (nGeneratedAmount2 - nFee);
				double dWeight = ((nGeneratedAmount - nFee)/ COIN) * (dDaysToStake - 8.8);
				
				entry.push_back(Pair("Stake TX Time", nTime));
				entry.push_back(Pair("Previous Time", nPrevTime));
				entry.push_back(Pair("Days To Stake", dDaysToStake));
				entry.push_back(Pair("Original Amount", ValueFromAmount(nGeneratedAmount - nFee)));
				entry.push_back(Pair("Weight", dWeight));
				entry.push_back(Pair("PoS Reward", ValueFromAmount(nFee)));
				entry.push_back(Pair("Reward %", percentReward));
				entry.push_back(Pair("Total New Amount", ValueFromAmount(nGeneratedAmount)));
				entry.push_back(Pair("Size of Each New Block", ValueFromAmount(nGeneratedAmount/2)));
			}
		}
    }
    return entry;
}

//presstab HyperStake
double getWeight()
{
	std::vector<COutput> vCoins;
    pwalletMain->AvailableCoins(vCoins);
	uint64 nWeightSum = 0;
	BOOST_FOREACH(const COutput& out, vCoins)
    {
		int64 nHeight = nBestHeight - out.nDepth;
		CBlockIndex* pindex = FindBlockByHeight(nHeight);
		uint64 nWeight = 0;
		pwalletMain->GetStakeWeightFromValue(out.tx->GetTxTime(), out.tx->vout[out.i].nValue, nWeight);
		int64 nAge = int64(GetTime() - pindex->nTime);
		int64 nStakeAge;
		if(fTestNet)
			nStakeAge = nStakeMinAge;
		else
			nStakeAge = nStakeMinAgeV2;
		
		if(nAge < nStakeAge)
			nWeight = 0;
		nWeightSum += nWeight;
	}
	return (double)nWeightSum;
}

//presstab HyperStake
Value getweight(const Array& params, bool fHelp)
{
	if (fHelp)
        throw runtime_error(
            "getweight\n"
            "This will return your total stake weight for confirmed outputs\n");
			
	return getWeight();
}

//presstab HyperStake
Value getpotentialstake(const Array& params, bool fHelp)
{
	 if (fHelp)
        throw runtime_error(
            "getpotentialstake\n"
            "This will return your total potential stake for confirmed outputs\n"
			"Potential stake is the amount your output reward is worth if it stakes right now");
			
	std::vector<COutput> vCoins;
    pwalletMain->AvailableCoins(vCoins);
	
	double nRewardSum = 0;
	BOOST_FOREACH(const COutput& out, vCoins)
    {
		int64 nHeight = nBestHeight - out.nDepth;
		CBlockIndex* pindex = FindBlockByHeight(nHeight);
		uint64 nAmount = out.tx->vout[out.i].nValue;
		double dAge = double(GetTime() - pindex->nTime) / (60*60*24);
		double nReward = 7.5 / 365 * dAge * (double)nAmount;
		nReward = min(nReward  / COIN, double(1000));
		nRewardSum += nReward;
	}
	return nRewardSum;
}

// presstab HyperStake
Value setstakesplitthreshold(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "setstakesplitthreshold <1 - 999,999>\n"
            "This will set the output size of your stakes to never be below this number\n");
    uint64 nStakeSplitThreshold = params[0].get_int();
	if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Unlock wallet to use this feature");
	if (nStakeSplitThreshold > 999999)
		return "out of range - setting split threshold failed";
	
	CWalletDB walletdb(pwalletMain->strWalletFile);
	LOCK(pwalletMain->cs_wallet);
	{
		bool fFileBacked = pwalletMain->fFileBacked;
		
		Object result;
		pwalletMain->nStakeSplitThreshold = nStakeSplitThreshold;
		result.push_back(Pair("split stake threshold set to ", int(pwalletMain->nStakeSplitThreshold)));
		if(fFileBacked)
		{
			walletdb.WriteStakeSplitThreshold(nStakeSplitThreshold);
			result.push_back(Pair("saved to wallet.dat ", "true"));
		}
		else
			result.push_back(Pair("saved to wallet.dat ", "false"));
		
		return result;
	}
}

// presstab HyperStake
Value getstakesplitthreshold(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getstakesplitthreshold\n"
            "Returns the set splitstakethreshold\n");

	Object result;
	result.push_back(Pair("split stake threshold set to ", int(pwalletMain->nStakeSplitThreshold)));
	return result;

}

// presstab HyperStake
Value disablestake(const Array& params, bool fHelp)
{
    if (fHelp || params.size()  > 4 || params.size() < 1)
        throw runtime_error(
            "disablestake <true/false>\n"
            "This will disable staking if set true\n"
			"example: disablestake true diff >5\n"
			"options: diff, weight");
    int fDisableStake = params[0].get_bool();
	
	if (params.size() == 4)
	{
		std::string strType = params[1].get_str();
		std::string strArg = params[2].get_str();
		double dUserNumber = params[3].get_real();
		
		if(strType == "diff" || "weight")
			pwalletMain->strDisableType = strType;
		else
			 throw runtime_error("type is not valid");
		if(strArg == ">" || "<")
			pwalletMain->strDisableArg = strArg;
		else
			 throw runtime_error("argument is not valid");
		
		
		pwalletMain->dUserNumber = dUserNumber;
		pwalletMain->fStakeRequirement = true;
	}
	else
		pwalletMain->fStakeRequirement = false;
	
	pwalletMain->fDisableStake = fDisableStake;
	
	if(!fDisableStake || params.size() == 1)
	{
		pwalletMain->strDisableType = "";
		pwalletMain->strDisableArg = "";
		pwalletMain->dUserNumber = 0;
		pwalletMain->fStakeRequirement = false;
	}
	
	Object result;
	result.push_back(Pair("disablestake ", pwalletMain->fDisableStake));
	result.push_back(Pair("stake requirements set? ", pwalletMain->fStakeRequirement));
	return result;
}

// presstab HyperStake
Value rescanfromblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "rescanfromblock <block height>\n"
            "This will rescan for transactions after a specified block\n");
    int nHeight = params[0].get_int();
	if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Unlock wallet to use this feature");
	if (nHeight > int(nBestHeight) || nHeight < 0)
		return "out of range";
		
	pwalletMain->ScanForWalletTransactions(FindBlockByHeight(nHeight), true);
	return "done";
}

// presstab HyperStake
Value cclistcoins(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "cclistcoins\n"
			"CoinControl: list your spendable coins and their information\n");
	
	Array result;
	
	std::vector<COutput> vCoins;
    pwalletMain->AvailableCoins(vCoins);
	
	BOOST_FOREACH(const COutput& out, vCoins)
    {
		Object coutput;
		int64 nHeight = nBestHeight - out.nDepth;
		CBlockIndex* pindex = FindBlockByHeight(nHeight);
		
		CTxDestination outputAddress;
		ExtractDestination(out.tx->vout[out.i].scriptPubKey, outputAddress);
		coutput.push_back(Pair("Address", CBitcoinAddress(outputAddress).ToString()));
		coutput.push_back(Pair("Output Hash", out.tx->GetHash().ToString()));
		coutput.push_back(Pair("blockIndex", out.i));
		double dAmount = double(out.tx->vout[out.i].nValue) / double(COIN);
		coutput.push_back(Pair("Value", dAmount));
		coutput.push_back(Pair("Confirmations", int(out.nDepth)));
		double dAge = double(GetTime() - pindex->nTime);
		coutput.push_back(Pair("Age (days)", (dAge/(60*60*24))));
		uint64 nWeight = 0;
		pwalletMain->GetStakeWeightFromValue(out.tx->GetTxTime(), out.tx->vout[out.i].nValue, nWeight);
		if(dAge < (fTestNet ? nStakeMinAge : nStakeMinAgeV2))
			nWeight = 0;
		coutput.push_back(Pair("Weight", int(nWeight)));
		double nReward = 7.5 / 365 * dAge/(60*60*24) * dAmount;
		nReward = min(nReward, double(1000));
		coutput.push_back(Pair("Potential Stake", nReward));
		result.push_back(coutput);
	}
	return result;
}


// presstab HyperStake
Value ccselect(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "ccselect <Output Hash> <Output Index>\n"
			"CoinControl: select a coin");
	
	uint256 hash;
    hash.SetHex(params[0].get_str());
	unsigned int nIndex = params[1].get_int();
	COutPoint outpt(hash, nIndex);
	coinControl->Select(outpt);

	return "Outpoint Selected";
}

// presstab HyperStake
Value cclistselected(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "cclistselected\n"
			"CoinControl: list selected coins");
	
	std::vector<COutPoint> vOutpoints;
	coinControl->ListSelected(vOutpoints);
	
	Array result;
	BOOST_FOREACH(COutPoint& outpt, vOutpoints)
	{
		result.push_back(outpt.hash.ToString());
	}

	return result;
}

// ssta HyperStake
Value ccreturnchange(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "ccreturnchange <true|false>\n"
                        "CoinControl: sets returnchange to true or false");
    bool rc = params[0].get_bool();
    coinControl->fReturnChange=rc;
    string ret = "Set ReturnChange to: ";
    
	if(coinControl->fReturnChange )
		ret+= "true";
	else
		ret+= "false";
		
    return ret;
}

// ssta HyperStake
Value cccustomchange(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "cccustomchange <address>\n"
                        "CoinControl: sets address to return change to");
    CBitcoinAddress address(params[0].get_str());
    // check it's a valid address
    if(!address.IsValid()) throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid HyperStake address");

    coinControl->destChange=address.Get();

    string ret = "Set change address to: ";
    ret+=(CBitcoinAddress(coinControl->destChange).ToString());
    return ret;
}

// ssta HyperStake
Value ccreset(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "ccreset\n"
                        "CoinControl: resets coin control (clears selected coins and change address)");
    coinControl->SetNull();
    return Value::null;
}

// presstab HyperStake
Value ccsend(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
		"ccsend <HyperStakeaddress> <amount>\n"
            "<amount> is a real and is rounded to the nearest 0.000001"
            + HelpRequiringPassphrase());

    EnsureWalletIsUnlocked();

    CBitcoinAddress address(params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid HyperStake address");

    // Amount
    int64 nAmount = AmountFromValue(params[1]);

    if (nAmount < MIN_TXOUT_AMOUNT)
        throw JSONRPCError(-101, "Send amount too small");

    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");

	// Initialize things needed for the transaction
   vector<pair<CScript, int64> > vecSend;
	CWalletTx wtx;
    CReserveKey keyChange(pwalletMain);
    int64 nFeeRequired = 0;
	CScript scriptPubKey;
        scriptPubKey.SetDestination(address.Get());
    vecSend.push_back(make_pair(scriptPubKey, nAmount));
	
    bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired, 1, false, coinControl); // 1 = no splitblock, false for s4c, coinControl
    if (!fCreated)
    {
        if (nAmount + nFeeRequired > pwalletMain->GetBalance())
            throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction creation failed");
    }
    if (!pwalletMain->CommitTransaction(wtx, keyChange))
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction commit failed");
	
	coinControl->SetNull();
    return wtx.GetHash().GetHex();
}


//presstab HyperStake
Array printMultiSend()
{
	Array ret;
	Object act;
	act.push_back(Pair("MultiSend Activated?", pwalletMain->fMultiSend));
	act.push_back(Pair("MultiSend in CoinStake?", pwalletMain->fMultiSendCoinStake));
	ret.push_back(act);
	if(pwalletMain->vDisabledAddresses.size() >= 1)
	{
		Object disAdd;
		for(unsigned int i = 0; i < pwalletMain->vDisabledAddresses.size(); i++)
		{
			disAdd.push_back(Pair("Disabled From Sending", pwalletMain->vDisabledAddresses[i]));
		}
		ret.push_back(disAdd);
	}
	
	ret.push_back("MultiSend Addresses to Send To:");
	Object vMS;
	for(unsigned int i = 0; i < pwalletMain->vMultiSend.size(); i++)
	{
		vMS.push_back(Pair("Address " + boost::lexical_cast<std::string>(i), pwalletMain->vMultiSend[i].first));
		vMS.push_back(Pair("Percent", pwalletMain->vMultiSend[i].second));
	}
	ret.push_back(vMS);
	return ret;
}

//presstab HyperStake
Array printAddresses()
{
	std::vector<COutput> vCoins;
    pwalletMain->AvailableCoins(vCoins);
	std::map<std::string, double> mapAddresses;
	
	BOOST_FOREACH(const COutput& out, vCoins)
    {
		CTxDestination utxoAddress;
		ExtractDestination(out.tx->vout[out.i].scriptPubKey, utxoAddress);
		std::string strAdd = CBitcoinAddress(utxoAddress).ToString();

		if(mapAddresses.find(strAdd) == mapAddresses.end()) //if strAdd is not already part of the map
		{
			mapAddresses[strAdd] = (double)out.tx->vout[out.i].nValue / (double)COIN;
		}
		else
		{
			mapAddresses[strAdd] += (double)out.tx->vout[out.i].nValue / (double)COIN;
		}
	}
	Array ret;
	for (map<std::string, double>::const_iterator it = mapAddresses.begin(); it != mapAddresses.end(); ++it)
	{
		Object obj;
		const std::string* strAdd = &(*it).first;
		const double* nBalance = &(*it).second;
		obj.push_back(Pair("Address ", *strAdd));
		obj.push_back(Pair("Balance ", *nBalance));
		ret.push_back(obj);
	}
	return ret;
}

//presstab HyperStake
unsigned int sumMultiSend()
{
	unsigned int sum = 0;
	for(unsigned int i = 0; i < pwalletMain->vMultiSend.size(); i++)
	{
		sum += pwalletMain->vMultiSend[i].second;
	}
	return sum;
}

// presstab HyperStake
Value multisend(const Array &params, bool fHelp)
{
    CWalletDB walletdb(pwalletMain->strWalletFile);
	bool fFileBacked;
	
	//MultiSend Commands
	if(params.size() == 1)
	{
		string strCommand = params[0].get_str();
		Object ret;
		if(strCommand == "print")
		{
			return printMultiSend();
		}
		else if(strCommand == "printaddress" || strCommand == "printaddresses")
		{
			return printAddresses();
		}
		else if(strCommand == "clear")
		{
			LOCK(pwalletMain->cs_wallet);
			{
				fFileBacked = pwalletMain->fFileBacked;
				string strRet;
				if(fFileBacked)
				{
					if(walletdb.EraseMultiSend(pwalletMain->vMultiSend))						
						strRet += "erased MultiSend vector from database & ";
					
				}
				pwalletMain->vMultiSend.clear();
				pwalletMain->fMultiSend = false;
				strRet += "cleared MultiSend vector from RAM";
				return strRet;
			}
		}
		else if (strCommand == "enable" || strCommand == "activate" )
		{
			if(pwalletMain->vMultiSend.size() < 1)
				return "Unable to activate MultiSend, check MultiSend vector";
			if(CBitcoinAddress(pwalletMain->vMultiSend[0].first).IsValid())
			{
				pwalletMain->fMultiSend = true;
				if(!walletdb.WriteMSettings(true, pwalletMain->nLastMultiSendHeight))
					return "MultiSend activated but writing settings to DB failed";
				else
				return "MultiSend activated";
			}
			else
				return "Unable to activate MultiSend, check MultiSend vector";
		}
		else if (strCommand == "disable" || strCommand == "deactivate" )
		{
			pwalletMain->fMultiSend = false;
			if(!walletdb.WriteMSettings(false, pwalletMain->nLastMultiSendHeight))
					return "MultiSend deactivated but writing settings to DB failed";
			return "MultiSend deactivated";
		}
		else if(strCommand == "enableall")
		{
			if(!walletdb.EraseMSDisabledAddresses(pwalletMain->vDisabledAddresses))
				return "failed to clear old vector from walletDB";
			else
			{
				pwalletMain->vDisabledAddresses.clear();
				return "all addresses will now send MultiSend transactions";
			}
		}
	}
	if(params.size() == 2 && params[0].get_str() == "delete")
	{
		int del = boost::lexical_cast<int>(params[1].get_str());
		if(!walletdb.EraseMultiSend(pwalletMain->vMultiSend))
		   return "failed to delete old MultiSend vector from database";
		
		pwalletMain->vMultiSend.erase(pwalletMain->vMultiSend.begin() + del);
		
		if(!walletdb.WriteMultiSend(pwalletMain->vMultiSend))
			return "walletdb WriteMultiSend failed!";
		return printMultiSend();
	}
	if(params.size() == 2 && params[0].get_str() == "disable")
	{
		std::string disAddress = params[1].get_str();
		if(!CBitcoinAddress(disAddress).IsValid())
			return "address you want to disable is not valid";
		else
		{
			pwalletMain->vDisabledAddresses.push_back(disAddress);
			if(!walletdb.EraseMSDisabledAddresses(pwalletMain->vDisabledAddresses))
				return "disabled address from sending, but failed to clear old vector from walletDB";
			if(!walletdb.WriteMSDisabledAddresses(pwalletMain->vDisabledAddresses))
				return "disabled address from sending, but failed to store it to walletDB";
			else
				return "disabled address from sending MultiSend transactions";
		}
		
	}
	if(params.size() == 2 && params[0].get_str() == "coinstake")
	{
		std::string strCoinStake = params[1].get_str();
		if(strCoinStake == "true")
		{
			pwalletMain->fMultiSendCoinStake = true;
			if(walletdb.WriteMCoinStake(true))
				return "MultiSend CoinStake enabled and saved to wallet DB";
			else
				return "MultiSend CoinStake enabled but failed to save to wallet DB";
		}
		else if(strCoinStake == "false")
		{
			pwalletMain->fMultiSendCoinStake = false;
			if(walletdb.WriteMCoinStake(false))
				return "MultiSend CoinStake disabled and saved to wallet DB";
			else
				return "MultiSend CoinStake edisabled but failed to save to wallet DB";
		}	
		else
			return "Did not recognize parameter";
	}
	//if no commands are used
	if (fHelp || params.size() != 2)
        throw runtime_error(
			"multisend <command>\n"
			"****************************************************************\n"
			"WHAT IS MULTISEND?\n"
			"MultiSend is a rebuild of what used to be called Stake For Charity (s4c)\n"
			"MultiSend allows a user to automatically send a percent of their stake reward to as many addresses as you would like\n"
			"The MultiSend transaction is sent when the staked coins mature (30 confirmations)\n"
			"The only current restriction is that you cannot choose to send more than 100% of your stake using MultiSend\n"
			"****************************************************************\n"
			"MULTISEND COMMANDS (usage: multisend <command>)\n"
			"   print - displays the current MultiSend vector \n"
			"   clear - deletes the current MultiSend vector \n"
			"   enable/activate - activates the current MultiSend vector \n"
			"   disable/deactivate - disables the current MultiSend vector \n"
			"   delete <Address #> - deletes an address from the MultiSend vector \n"
			"   disable <address> - prevents a specific address from sending MultiSend transactions\n"
			"   enableall - enables all addresses to be eligible to send MultiSend transactions\n"
			"   coinstake <true/false> - this will send the multisend transaction in the coinstake transaction\n"
			
			"****************************************************************\n"
			"TO CREATE OR ADD TO THE MULTISEND VECTOR:\n"
			"multisend <HyperStake Address> <percent>\n"
            "This will add a new address to the MultiSend vector\n"
            "Percent is a whole number 1 to 100.\n"
			"****************************************************************\n"
            );
	
	//if the user is entering a new MultiSend item
	string strAddress = params[0].get_str();
    CBitcoinAddress address(strAddress);
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid HyperStake address");
    if (boost::lexical_cast<int>(params[1].get_str()) < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected valid percentage");
    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");

	
	unsigned int nPercent = boost::lexical_cast<unsigned int>(params[1].get_str());
    
	LOCK(pwalletMain->cs_wallet);
	{
		fFileBacked = pwalletMain->fFileBacked;
		//Error if 0 is entered
		if(nPercent == 0)
		{
            return "Sending 0% of stake is not valid";
		}
		
		//MultiSend can only send 100% of your stake
        if (nPercent + sumMultiSend() > 100)
			return "Failed to add to MultiSend vector, the sum of your MultiSend is greater than 100%";
		
		for(unsigned int i = 0; i < pwalletMain->vMultiSend.size(); i++)
		{
			if(pwalletMain->vMultiSend[i].first == strAddress)
				return "Failed to add to MultiSend vector, cannot use the same address twice";
		}
			
		if(fFileBacked)
			walletdb.EraseMultiSend(pwalletMain->vMultiSend);
			  
		std::pair<std::string, int> newMultiSend;
		newMultiSend.first = strAddress;
		newMultiSend.second = nPercent;
		pwalletMain->vMultiSend.push_back(newMultiSend);
		
		if(fFileBacked)
		{	
			if(!walletdb.WriteMultiSend(pwalletMain->vMultiSend))
				return "walletdb WriteMultiSend failed!";
		}
	}
	return printMultiSend();
}

// presstab HyperStake
Value strictprotocol(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "strictprotocol <true/false>\n"
				"Refuse connections to other protocols\n"
                "WARNING: if set true peer count will drop\n");
    fStrictProtocol = params[0].get_bool();
	if(fStrictProtocol)
		return "Strict Protocol True";
	else
		return "Strict Protocol False";
}

// presstab HyperStake
Value strictincoming(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "strictincoming <true/false>\n"
			"drop connections from peers sending too many orphans or redundant requests"
              "WARNING: if set true peer count will drop");
    fStrictIncoming = params[0].get_bool();
	if(fStrictIncoming)
		return "Strict Incoming True";
	else
		return "Strict Incoming False";
}	
	
// presstab HyperStake
Value hashsettings(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2 || params.size() == 0)
        throw runtime_error(
            "hashsettings <drift/interval><seconds>\n"
			"hashsettings <combineduse><true/false>\n"
			"ex: 'hashsettings drift' will return the current drift settings\n"
			"ex: 'hashsettings interval' will return the current interval settings\n"
			"ex: hashsettings drift 45\n"
			"ex: hashsettings interval 20\n"
			"ex: 'hashsettings default' returns the settings to default\n"
			"ex: 'hashsettings combinedust true' dust outputs will be combined when staking if eligible\n"
			"hashdrift is how many second into the future your wallet will stake in one hashing burst\n"
			"interval is how often your client will search for new hashes\n"
			"if you set your hashdrift to 45, then your client will create 45 unique proof of stake hashes, the only thing changing the hash result is the timestamp included, thus you hash 45 seconds into the future.\n"
			"Each hash is an attempt to meet the staking target. If you don't hit the staking target, your client will pause staking for the set interval. \n"
			"If the interval is 22 seconds, the wallet will create 45 hashes, wait 22 seconds, then create 45 hashes. Approximately 23 of those hashes will be identical as the previous rounds hashes.\n"
              "WARNING: timedrift allowance is 60 seconds too high of a hash drift will cause orphans\n"
			  "Combine dust is a setting in the staking parameters that will iterate through your entire wallet contents to looks for small coins that it can combine into your coinstake transaction." 
			  "set this to false to prevent any combination from occurring \n");
    if(params.size() < 2)
	{
		if(params[0].get_str() == "drift") 
			return boost::lexical_cast<string>(pwalletMain->nHashDrift);
		else if(params[0].get_str() == "interval") 
			return boost::lexical_cast<string>(pwalletMain->nHashInterval);
		else if(params[0].get_str() == "default")
		{
			CWalletDB walletdb(pwalletMain->strWalletFile);
			pwalletMain->nHashDrift = 45;
			pwalletMain->nHashInterval = 22;
			walletdb.WriteHashDrift(45);
			walletdb.WriteHashInterval(22);
			return "Hash Settings returned to default";
		}
		else if(params[0].get_str() == "combinedust")
			return pwalletMain->fCombineDust;
	}
	
	CWalletDB walletdb(pwalletMain->strWalletFile);
	if(params[0].get_str() == "drift") 
	{
		unsigned int nHashDrift = boost::lexical_cast<unsigned int>(params[1].get_str());
		if(nHashDrift > 60)
			return "You can not set your hashdrift to be greater than 60 seconds";
		else if(nHashDrift < 1)
			return "Hash drift too low";
		
		pwalletMain->nHashDrift = nHashDrift;
		if(walletdb.WriteHashDrift(nHashDrift))
			return "Hashdrift set and save to DB";
		else
			return "Hashdrift set but failed to write to DB";
	}
	else if(params[0].get_str() == "interval") 
	{
		unsigned int nHashInterval = boost::lexical_cast<unsigned int>(params[1].get_str());
		if(nHashInterval < 1)
			return "Interval too low";
		pwalletMain->nHashInterval = nHashInterval;
		if(walletdb.WriteHashInterval(nHashInterval))
			return "HashInterval set and save to DB";
		else
			return "HashInterval set but failed to write to DB";
	}
	else if(params[0].get_str() == "combinedust")
	{
		bool fCombineDust;
		string strCombineDust = params[1].get_str();
		if(strCombineDust == "true")
			fCombineDust = true;
		else if(strCombineDust == "false")
			fCombineDust = false;
		else
			return "failed to understand true/false parameter";
		
		pwalletMain->fCombineDust = fCombineDust;
		if(walletdb.WriteCombineDust(fCombineDust))
			return "Combine dust setting saved and written to DB";
		else
			return "Combine dust setting saved and but failed to write to DB";
	}
	return "Failed to recognize commands";
}

// PIVX
Value getstakingstatus(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
                "getstakingstatus\n"
                        "Returns an object containing various staking information.\n"
                        "\nResult:\n"
                        "{\n"
                        "  \"generatebitcoins\": true|false,   (boolean) if the internal generate bitcoins flag is activated\n"
                        "  \"haveconnections\": true|false,    (boolean) if network connections are present\n"
                        "  \"blockchainsynced\": true|false,   (boolean) if blockchain is fully synced\n"
                        "  \"walletunlocked\": true|false,     (boolean) if the wallet is unlocked\n"
                        "  \"mintablecoins\": true|false,      (boolean) if the wallet has mintable coins\n"
                        "  \"enoughcoins\": true|false,        (boolean) if available coins are greater than reserve balance\n"
                        "  \"staking status\": true|false,     (boolean) if the wallet is staking or not\n"
                        "}\n");

    Object obj;
    obj.push_back(Pair("generatebitcoins", fGenerateBitcoins));
    obj.push_back(Pair("haveconnections", !vNodes.empty()));
    obj.push_back(Pair("blockchainsynced", !IsInitialBlockDownload()));
    if (pwalletMain) {
        obj.push_back(Pair("walletunlocked", !pwalletMain->IsLocked()));
        obj.push_back(Pair("mintablecoins", pwalletMain->MintableCoins()));

        int64 nReserveBalance = 0;
        if (mapArgs.count("-reservebalance") && !ParseMoney(mapArgs["-reservebalance"], nReserveBalance))
            nReserveBalance = 0;

        obj.push_back(Pair("enoughcoins", nReserveBalance <= pwalletMain->GetBalance()));
    }

    bool fStaking = false;
    if (mapHashedBlocks.count(nBestHeight))
        fStaking = true;
    else if (mapHashedBlocks.count(nBestHeight - 1) && nLastCoinStakeSearchInterval)
        fStaking = true;
    obj.push_back(Pair("staking status", fStaking));

    return obj;
}

// HyperStake
Value sendproposal(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "sendproposal <proposal hash>\n"
                "Sends a vote proposal to the network. WARNING this adds a transaction fee of 5 HYP!\n"
                "\nResult:\n"
                "{\n"
                "  \"txid\": hash,             (string) the hash of the transaction that was sent\n"
                "}\n");

    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Unlock wallet to use this feature");

    //! See if this proposal exists in our map of pending proposals
    uint256 hashProposal;
    hashProposal.SetHex(params[0].get_str());
    if (!mapPendingProposals.count(hashProposal))
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot find proposal");

    CTransaction tx = mapPendingProposals.at(hashProposal);
    CWalletTx wtx(pwalletMain, tx);

    //! Get available coins and add enough to cover the proposal fee
    vector<COutput> vCoins;
    pwalletMain->AvailableCoins(vCoins, true);

    int64 nFee = 5 * COIN;
    int64 nValueIn = 0;

    set<pair<const CWalletTx*,unsigned int> > setCoins;
    if (!pwalletMain->SelectCoinsMinConf(nFee, tx.nTime, 1, 6, vCoins, setCoins, nValueIn))
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");

    //! Fill vin
    for (pair<const CWalletTx*,unsigned int> coin : setCoins)
        wtx.vin.push_back(CTxIn(coin.first->GetHash(),coin.second));

    //! Add the min value required for an output to the proposal UTXO
    wtx.vout[0].nValue = MIN_TXOUT_AMOUNT;

    //! Figure out change amount
    nFee -= wtx.vout[0].nValue;
    int64 nChange = nValueIn - nFee - MIN_TXOUT_AMOUNT;
    if (nChange > 500) {
        //!Lookup the address of one of the inputs and return the change to that address
        uint256 hashBlock;
        CTransaction txPrev;
        if(!GetTransaction(wtx.vin[0].prevout.hash, txPrev, hashBlock))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to select coins");

        CScript scriptReturn = txPrev.vout[wtx.vin[0].prevout.n].scriptPubKey;
        CTxOut out(nChange, scriptReturn);

        //!Add the change output to the new transaction
        wtx.vout.push_back(out);
    }

    //! Sign the transaction
    int nIn = 0;
    for (const pair<const CWalletTx*,unsigned int>& coin : setCoins) {
        if (!SignSignature(*pwalletMain, *coin.first, wtx, nIn++))
            return false;
    }

    //! Broadcast the transaction to the network
    CReserveKey reserveKey = CReserveKey(pwalletMain);
    if (!pwalletMain->CommitTransaction(wtx, reserveKey))
        throw JSONRPCError(RPC_WALLET_ERROR, "Failed to commit transaction");

    Object ret;
    ret.push_back(Pair("txid", wtx.GetHash().GetHex()));

    return ret;
}

// HyperStake
Value setvote(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
                "setvote <txid> <number>\n"
                        "Creates a vote object for the proposal in the txid which will trigger when you Stake.\n"
                        "\nResult:\n"
                        "{\n"
                        "  \"Proposal Name\":,             (string) The name of the proposal\n"
                        "  \"Proposal Description\":,      (string) The description of the proposal\n"
                        "  \"Your vote\":,                 (int)    0 - Abstain, 1 - Yes, 2 - No, 3 - Request proposal revision\n"
                        "}\n");

    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Unlock wallet vote on a proposal");

    //Get params
    uint256 txHash(params[0].get_str());
    int voteChoice = params[1].get_int();

    CTransaction tx;
    uint256 hashBlock;
    if(!GetTransaction(txHash, tx, hashBlock))
        return "Transaction not found in wallet";

    if (!tx.IsProposal())
        return "Transaction does not contain a proposal";

    if (voteChoice > 3 || voteChoice < 0)
        return "You must vote on the following using a number from 0-3(inclusive)\n 0 - Abstain,\n 1 - Yes,\n 2 - No,\n 3 - Request proposal revision";

    CVoteProposal proposal;
    if (!ProposalFromTransaction(tx, proposal))
        return "Proposal couldn't be found in the transaction";

    CVoteObject voteObject(proposal.GetHash(), proposal.GetLocation());

    voteObject.Vote(voteChoice);

    //add the voteObject in the map
    pwalletMain->mapVoteObjects[proposal.GetHash()] = voteObject;

    //write the vote object to the database
    CWalletDB walletdb(pwalletMain->strWalletFile);
    if (!walletdb.WriteVoteObject(proposal.GetHash().GetHex(), voteObject)) {
        return "The vote was saved, however had problems writing the vote to the database";
    }

    Object ret;
    ret.push_back(Pair("Proposal Name", proposal.GetName()));
    ret.push_back(Pair("Proposal Description", proposal.GetDescription()));
    ret.push_back(Pair("Your Vote", voteChoice));
    return ret;
}

// HyperStake
Value getvote(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
                "getvote <txid> \n"
                        "Returns the vote you made on a proposal from the given txid\n"
                        "\nResult:\n"
                        "{\n"
                        "  \"Proposal Name\":,             (string) The name of the proposal\n"
                        "  \"Proposal Description\":,      (string) The description of the proposal\n"
                        "  \"Your vote\":,                 (int)    0 - Abstain, 1 - Yes, 2 - No, 3 - Request proposal revision\n"
                        "}\n");

    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Unlock wallet vote on a proposal");

    //Get params
    uint256 txHash(params[0].get_str());

    CTransaction tx;
    uint256 hashBlock;
    if (!GetTransaction(txHash, tx, hashBlock))
        return "Transaction not found in wallet";

    if (!tx.IsProposal())
        return "Transaction does not contain a proposal";

    CVoteProposal proposal;
    if (!ProposalFromTransaction(tx, proposal))
        return "Proposal couldn't be found in the transaction";

    uint256 proposalHash(proposal.GetHash());

    // Check to see if we have the vote object loaded from the database, if we do return the info
    if (pwalletMain->mapVoteObjects.count(proposalHash) != 0) {
        CVoteObject voteObject = pwalletMain->mapVoteObjects[proposalHash];

        int voteValue = voteObject.GetUnformattedVote();

        Object ret;
        ret.push_back(Pair("Proposal Name", proposal.GetName()));
        ret.push_back(Pair("Proposal Name", proposal.GetName()));
        ret.push_back(Pair("Proposal Description", proposal.GetDescription()));
        ret.push_back(Pair("Your Vote", voteValue));
        ret.push_back(Pair("Proposal Hash", proposal.GetHash().GetHex()));
        return ret;
    }

    // Try and find the voteobject in the walletdb and return it if found
    CWalletDB walletdb(pwalletMain->strWalletFile);

    CVoteObject voteObject;
    if (!walletdb.ReadVoteObject(proposal.GetHash().GetHex(), voteObject))
        return "No vote has been stored for this proposal ";

    int voteValue = voteObject.GetUnformattedVote();

    Object ret;
    ret.push_back(Pair("Proposal Name", proposal.GetName()));
    ret.push_back(Pair("Proposal Description", proposal.GetDescription()));
    ret.push_back(Pair("Your Vote", voteValue));
    ret.push_back(Pair("Proposal Hash", proposal.GetHash().GetHex()));
    return ret;
}

// HyperStake
Value getvotes(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
                "getvotes \n"
                        "Returns all vote objects\n"
                        "\nResult:\n"
                        "{\n"
                        "  \"Proposal Name\":,             (string) The name of the proposal\n"
                        "  \"Proposal Description\":,      (string) The description of the proposal\n"
                        "  \"Proposal Tx\":,               (string) The tx the proposal was started in\n"
                        "  \"Your vote\":,                 (int)    0 - Abstain, 1 - Yes, 2 - No, 3 - Request proposal revision\n"
                        "}\n");

    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Unlock wallet vote on a proposal");

    CVoteDB votedb("r");
    if (pwalletMain->mapVoteObjects.size() > 0) {
        Array ret;
        map<uint256, CVoteObject>::iterator it;
        for (it = pwalletMain->mapVoteObjects.begin(); it != pwalletMain->mapVoteObjects.end(); it++) {

            uint256 txid = 0;
            for (auto mit : mapProposals) {
                if (mit.second == it->first) {
                    txid = mit.first;
                    break;
                }
            }

            if (txid == 0)
                return JSONRPCError(RPC_DATABASE_ERROR, strprintf("failed to find txid for proposal %s", it->first.GetHex().c_str()));

            CVoteProposal proposal;
            if (!votedb.ReadProposal(txid, proposal)) {
                string strMessage = strprintf("Failed to find proposal %s in database", txid.GetHex().c_str());
                return JSONRPCError(RPC_DATABASE_ERROR, strMessage);
            }


            int voteValue = it->second.GetFormattedVote() >> proposal.GetShift() & proposal.GetBitCount();
            Object entry;
            entry.push_back(Pair("Proposal Name", proposal.GetName()));
            entry.push_back(Pair("Proposal Description", proposal.GetDescription()));
            entry.push_back(Pair("Your Vote", (int64_t)it->second.GetUnformattedVote()));
            ret.push_back(entry);
        }
        return ret;
    }
    return "You don't have any votes saved into the database";
}


