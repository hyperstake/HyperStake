// Copyright (c) 2017 The HyperStake Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "voteobject.h"
#include "main.h"

using namespace std;

bool CVoteObject::Vote(int nVotersChoice)
{
    nChoice = static_cast<uint32_t>(nVotersChoice);
    printf("shift %d\n", nShift);
    return true;
}

