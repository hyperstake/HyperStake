// Copyright (c) 2017 The HyperStake Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "voteobject.h"
#include "main.h"

using namespace std;

int CVoteObject::GetCombinedVotes(const std::vector<CVoteObject>& votes) {
    int nCombinedVotes = 0;

    for(CVoteObject vote: votes){
        nCombinedVotes |= vote.GetFormattedVote();
    }

    return nCombinedVotes;
}

bool CVoteObject::Vote(int nVotersChoice)
{
    nChoice = static_cast<uint32_t>(nVotersChoice);
    return true;
}

