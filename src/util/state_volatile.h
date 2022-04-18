#ifndef STATE_VOLATILE_H
#define STATE_VOLATILE_H

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
class VolatileState
{
private:
    int _commitIndex = -1;
    int _lastAppliedIndex = -1;
    unordered_map<string, int> _matchIndex;
    unordered_map<string, int> _nextIndex;
    int _identity = FOLLOWER;

public:
    // For all servers
    void SetCommitIndex(int index);
    int GetCommitIndex();
    void SetLastAppliedIndex(int index);
    int GetLastAppliedIndex();
    void SetIdentity(int identity);
    int GetIdentity();

    // Only for leader
    void SetNextIndex(string serverId, int value);
    int GetNextIndex(string serverId);
    void SetMatchIndex(string serverId, int value);
    int GetMatchIndex(string serverId);
};

#endif