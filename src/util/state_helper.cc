// TODO: Add to cmake
#include "state_helper.h"


StateHelper::StateHelper()
{
    Init();
}


/*
*   @brief Get the current term from mem
*/
int StateHelper::GetCurrentTerm()
{
    return vTermVoteObj.GetCurrentTerm();
}

/*
*   @brief Add node ip vote for current term in mem and persistent
*
*   @param term 
*   @param ip 
*/
void StateHelper::AddVotedFor(int term, string ip)
{
    if (!vTermVoteObj.HasVoted(term))
    {
        pTermVoteObj.AddVotedFor(term, ip);
        vTermVoteObj.AddVotedFor(term, ip);
    }
}

/*
*   @brief Get voted node ip for specified term
*
*   @param term 
*   @return ip 
*/
string StateHelper::GetVotedFor(int term)
{
    return vTermVoteObj.GetVotedFor(term);
}

/*
*   @brief Update current term in mem and persist
*
*   @param term 
*/
void StateHelper::AddCurrentTerm(int term)
{
    vTermVoteObj.UpdateCurrentTerm(term);
    pTermVoteObj.AddTerm(term);
}

/*
*   @brief Add log entry to the end
*
*   @param term
*   @param key
*   @param value
*/
void StateHelper::Append(int term, string key, string value)
{
    dbgprintf("[DEBUG]: Append - Entering function\n");
    int offset = 0;

    offset = pReplicatedLogObj.GetEndOfFileOffset();
    pReplicatedLogObj.Append(term, key, value, offset);
    vReplicatedLogObj.Append(term, key, value, offset);

    dbgprintf("[DEBUG]: Append - Exiting function\n");
}

/*
*   @brief Add log entry(ies) from the specified index
*
*   @param start index
*   @param entries (term, key, value
*/
// TODO: Make cleaner - append is being called in Insert
void StateHelper::Insert(int start_index, vector<Entry> &entries)
{
    dbgprintf("[DEBUG]: Insert - Entering function\n");
    int offset = 0;
    int index = 0;

    index = start_index;
    if(index==GetLogLength())
    {
        dbgprintf("Inside latest INSERT function\n");
        for(auto val : entries)
        {
            Append(val.term, val.key, val.value);
        }
        dbgprintf("Done with the latest INSERT function\n");
        return;
    }
    offset = vReplicatedLogObj.GetOffset(start_index);
    
    if (offset == -1)
    {
        dbgprintf("[DEBUG]: Insert - offset == -1\n");
        dbgprintf("[DEBUG]: Insert - Exiting function\n");
        return;
    }
    
    // preserve log up to offset bytes
    if (truncate(REPLICATED_LOG_PATH, offset) == -1)
    {
        throw runtime_error("[ERROR]: truncate failed\n");
    }

    for (auto val : entries)
    {
        dbgprintf("[DEBUG]: Insert - offset = %d\n", offset);

        pReplicatedLogObj.Insert(offset, val.term, val.key, val.value);
        vReplicatedLogObj.Insert(index, val.term, val.key, val.value, offset);
        
        offset = pReplicatedLogObj.GetCurrentFileOffset();
        index += 1;
    }

    dbgprintf("[DEBUG]: Insert - Exiting function\n");
}

/*
*   @brief Get the number of total entries in the log
*/
int StateHelper::GetLogLength()
{
    return vReplicatedLogObj.GetLength();
}

/*
*   @brief Get the term number at the specified index
*
*   @param index
*/
int StateHelper::GetTermAtIndex(int index)
{
    return vReplicatedLogObj.GetTerm(index);
}

/*
*   @brief Get the key of the command at the specified index
*
*   @param index
*/
string StateHelper::GetKeyAtIndex(int index)
{
    return vReplicatedLogObj.GetKey(index);
}

/*
*   @brief Get the value of the command at the specified index
*
*   @param index
*/
string StateHelper::GetValueAtIndex(int index)
{
    return vReplicatedLogObj.GetValue(index);
}

/*
*   @brief Parse logs
*/
void StateHelper::Init()
{
    // Term vote log
    auto tv_entries = pTermVoteObj.ParseLog();
    int currentTerm = 0;
    for (auto entry : tv_entries)
    {
        if (entry.votedFor != "") vTermVoteObj.AddVotedFor(entry.term, entry.votedFor);
        if (entry.term > currentTerm) currentTerm = entry.term;
    }

    if (GetLogLength() == 0)
    {
        Append(0, "NULL", "NULL");
    }   

    vTermVoteObj.UpdateCurrentTerm(currentTerm);

    // Replicated log
    auto rl_entries = pReplicatedLogObj.ParseLog();

    for (auto entry : rl_entries)
    {
        vReplicatedLogObj.Append(entry.term, entry.key, entry.value, entry.offset); 
    }

    // Set volatile states
    SetCommitIndex(0);
    SetLastAppliedIndex(0);
}

/*
*   @brief Update commit index in mem
*
*   @param index 
*/
void StateHelper::SetCommitIndex(int index)
{
    vStateObj.SetCommitIndex(index);
}


/* 
*   @brief Retrieve commit index from mem
*
*   @return commit index 
*/
int StateHelper::GetCommitIndex()
{
    return vStateObj.GetCommitIndex();
}

/*
*   @brief Update index of latest executed command in mem
*
*   @param index 
*/
void StateHelper::SetLastAppliedIndex(int index)
{
    vStateObj.SetLastAppliedIndex(index);
}

/*
*   @brief Get index of latest executed command from mem
*
*   @return index 
*/
int StateHelper::GetLastAppliedIndex()
{
    return vStateObj.GetLastAppliedIndex();
}

/*
*   @brief Set identity of server to Leader/Follower/Candidate in mem
*           Note: Use the ServerIdentity enum in common.h
*
*   @param identity 
*/
void StateHelper::SetIdentity(int identity)
{
    int prevIdentity = vStateObj.GetIdentity();

    if ((identity == LEADER && prevIdentity == FOLLOWER))
    {
        throw runtime_error("[ERROR]: Invalid state transition");
    }

    vStateObj.SetIdentity(identity);
}

/*
*   @brief Get identity of server from mem
*/
int StateHelper::GetIdentity()
{
    return vStateObj.GetIdentity();
}

/*
*   @brief Set next index (value) of particular serverId in mem
*
*   @param serverId 
*   @param value 
*/
void StateHelper::SetNextIndex(string serverId, int value)
{
    vStateObj.SetNextIndex(serverId, value);
}

/*
*   @brief Get next index (value) of particular serverId from mem
*
*   @param serverId 
*   @return value 
*/
int StateHelper::GetNextIndex(string serverId)
{
    dbgprintf("[DEBUG]: %s\n", __func__);
    return vStateObj.GetNextIndex(serverId);
}

/*
*   @brief Set match index (value) of particular serverId in mem
*
*   @param serverId 
*   @param value 
*/
void StateHelper::SetMatchIndex(string serverId, int value)
{
    vStateObj.SetMatchIndex(serverId, value);
}

/*
*   @brief Get next index (value) of particular serverId from mem
*
*   @param serverId 
*   @return value 
*/
int StateHelper::GetMatchIndex(string serverId)
{
    dbgprintf("[DEBUG]: %s\n", __func__);
    return vStateObj.GetMatchIndex(serverId);
}

/*
*   @brief Uncomment to test individually
*   @usage g++ state_helper.cc term_vote_volatile.cc 
*          term_vote_persistent.cc replicated_log_persistent.cc 
*          replicated_log_volatile.cc state_volatile.cc  -Wall -o state 
*          && ./state
*/
// int main()
// {
//     StateHelper obj;

//     // Test Init
//     obj.Init();

//     // Test Term Vote
//     cout << "Current term = " << obj.GetCurrentTerm() << endl;
    
//     obj.AddCurrentTerm(1);
//     cout << "Current term = " << obj.GetCurrentTerm() << endl;
//     cout << "voted for in term 1 = " << obj.GetVotedFor(1) << endl;

//     obj.AddVotedFor(1, "node");
//     obj.AddVotedFor(1, "node");
//     cout << "voted for in term 1 = " << obj.GetVotedFor(1) << endl;

//     // Test Append
//     obj.Append(1, "key1", "value");
//     dbgprintf("Log length = %d\n", obj.GetLogLength());

//     obj.Append(2, "key2", "value");
//     dbgprintf("Log length = %d\n", obj.GetLogLength());

//     obj.Append(3, "key3", "value");
//     dbgprintf("Log length = %d\n", obj.GetLogLength());

//     // Test Insert
//     vector<Entry> entries;
//     entries.push_back(Entry(1, "a", "d"));
//     entries.push_back(Entry(2, "b", "e"));
//     obj.Insert(1, entries);
//     dbgprintf("Log length = %d\n", obj.GetLogLength());

//     // Test Get term
//     dbgprintf("Term at index %d is %d\n", 1, obj.GetTermAtIndex(1));
//     dbgprintf("Term at index %d is %d\n", 2, obj.GetTermAtIndex(2));

//     return 0;
// }
