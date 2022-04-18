/*
*   Replicated Log Format:
*   <term>,<key>,<value>,<offset at which this entry exist in file>
*
*   Term Vote Log Format:
*   <term>,<ip of voted for node>
*/

#ifndef STATE_HELPER_H
#define STATE_HELPER_H

#include "common.h"
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include "term_vote_persistent.h"
#include "term_vote_volatile.h"

/******************************************************************************
 * GLOBALS
 *****************************************************************************/


/******************************************************************************
 * MACROS
 *****************************************************************************/


/******************************************************************************
 * NAMESPACES
 *****************************************************************************/



/******************************************************************************
 * DECLARATION
 *****************************************************************************/
class StateHelper
{
private:
    PersistentTermVote pTermVoteObj;
    VolatileTermVote vTermVoteObj;
    
public:
    int GetCurrentTerm();
    void AddVotedFor(int term, string ip);
    string GetVotedFor(int term);
    void AddCurrentTerm(int term);
    void Init();
};


#endif
