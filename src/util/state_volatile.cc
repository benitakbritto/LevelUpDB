// TODO: Add to cmake
#include "state_volatile.h"

/*
*   @brief Update commit index in mem
*
*   @param index 
*/
void VolatileState::SetCommitIndex(int index)
{
    _commitIndex = index;
}

/* 
*   @brief Retrieve commit index from mem
*
*   @return commit index 
*/
int VolatileState::GetCommitIndex()
{
    return _commitIndex;
}

/*
*   @brief Update index of latest executed command in mem
*
*   @param index 
*/
void VolatileState::SetLastAppliedIndex(int index)
{
    _lastAppliedIndex = index;
}

/*
*   @brief Get index of latest executed command from mem
*
*   @return index 
*/
int VolatileState::GetLastAppliedIndex()
{
    return _lastAppliedIndex;
}

/*
*   @brief Set identity of server to Leader/Follower/Candidate in mem
*           Note: Use the ServerIdentity enum in common.h
*
*   @param identity 
*/
void VolatileState::SetIdentity(int identity)
{
    _identity = identity;
}

/*
*   @brief Get identity of server from mem
*/
int VolatileState::GetIdentity()
{
    return _identity;
}

/*
*   @brief Set next index (value) of particular serverId in mem
*
*   @param serverId 
*   @param value 
*/
void VolatileState::SetNextIndex(string serverId, int value)
{
    _nextIndex[serverId] = value;
}

/*
*   @brief Get next index (value) of particular serverId from mem
*
*   @param serverId 
*   @return value 
*/
int VolatileState::GetNextIndex(string serverId)
{
    if (_nextIndex.count(serverId) == 0)
    {
        throw runtime_error("[ERROR]: Invalid index");
    }

    return _nextIndex[serverId];
}

/*
*   @brief Set match index (value) of particular serverId in mem
*
*   @param serverId 
*   @param value 
*/
void VolatileState::SetMatchIndex(string serverId, int value)
{
    _matchIndex[serverId] = value;
}

/*
*   @brief Get next index (value) of particular serverId from mem
*
*   @param serverId 
*   @return value 
*/
int VolatileState::GetMatchIndex(string serverId)
{
    if (_matchIndex.count(serverId) == 0)
    {
        throw runtime_error("[ERROR]: Invalid index");
    }

    return _matchIndex[serverId];
}

/*
*   @brief Uncomment to test individually
*   @usage g++ -o sv state_volatile.cc -Wall
*/
// int main()
// {
//     VolatileState obj;

//     cout << "Commit index = " << obj.GetCommitIndex() << endl;
    
//     obj.SetCommitIndex(2);
//     cout << "Commit index = " << obj.GetCommitIndex() << endl;

//     cout << "Identity = " << obj.GetIdentity() << endl;

//     cout << "Next index = " << obj.GetNextIndex("1") << endl; // should fail

//     return 0;
// }
