#ifndef TERM_VOTE_VOLATILE_H
#define TERM_VOTE_VOLATILE_H

#include "common.h"
#include <unordered_map>

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
class VolatileTermVote
{
private:
    unordered_map<int, string> votedFor; // term, ip
    int current_term = 0;
    
public:
    void UpdateCurrentTerm(int term);
    int GetCurrentTerm();
    void AddVotedFor(int term, string ip);
    string GetVotedFor(int term);
    bool HasNotVoted(int term);
};

#endif