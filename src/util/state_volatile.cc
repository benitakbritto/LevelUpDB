// TODO: Add to cmake
#include "state_volatile.h"

void VolatileState::SetCommitIndex(int index)
{
    _commitIndex = index;
}

int VolatileState::GetCommitIndex()
{
    return _commitIndex;
}

void VolatileState::SetLastAppliedIndex(int index)
{
    _lastAppliedIndex = index;
}

int VolatileState::GetLastAppliedIndex()
{
    return _lastAppliedIndex;
}

void VolatileState::SetIdentity(int identity)
{
    _identity = identity;
}

int VolatileState::GetIdentity()
{
    return _identity;
}

void VolatileState::SetNextIndex(string serverId, int value)
{
    _nextIndex[serverId] = value;
}

int VolatileState::GetNextIndex(string serverId)
{
    if (_nextIndex.count(serverId) == 0)
    {
        throw runtime_error("[ERROR]: Invalid index");
    }

    return _nextIndex[serverId];
}

void VolatileState::SetMatchIndex(string serverId, int value)
{
    _matchIndex[serverId] = value;
}


int VolatileState::GetMatchIndex(string serverId)
{
    if (_matchIndex.count(serverId) == 0)
    {
        throw runtime_error("[ERROR]: Invalid index");
    }

    return _matchIndex[serverId];
}


// Tester
// g++ -o sv state_volatile.cc -Wall
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