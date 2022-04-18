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
#include "replicated_log_persistent.h"
#include "replicated_log_volatile.h"

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
struct ReplicatedLogEntry
{
    int term;
    string key;
    string value;
    ReplicatedLogEntry(int term, string key, string value)
            : term(term), key(key), value(value) {}
};

typedef ReplicatedLogEntry Entry;

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
    PersistentReplicatedLog pReplicatedLogObj;
    VolatileReplicatedLog vReplicatedLogObj;
    
public:
    void Init();

    // Term Vote
    int GetCurrentTerm();
    void AddVotedFor(int term, string ip);
    string GetVotedFor(int term);
    void AddCurrentTerm(int term);
    
    // Replicated Log
    void Append(int term, string key, string value);
    void Insert(int start_index, vector<Entry> &entries);
    int GetLogLength();
    int GetTermAtIndex(int index);
};


#endif
