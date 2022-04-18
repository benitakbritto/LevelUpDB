#include "state_helper.h"

int StateHelper::GetCurrentTerm()
{
    return vTermVoteObj.GetCurrentTerm();
}

void StateHelper::AddVotedFor(int term, string ip)
{
    if (vTermVoteObj.HasNotVoted(term))
    {
        pTermVoteObj.AddVotedFor(term, ip);
        vTermVoteObj.AddVotedFor(term, ip);
    }
}

string StateHelper::GetVotedFor(int term)
{
    return vTermVoteObj.GetVotedFor(term);
}


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
void StateHelper::Insert(int start_index, vector<Entry> &entries)
{
    dbgprintf("[DEBUG]: Insert - Entering function\n");
    int offset = 0;
    int index = 0;

    index = start_index;
    offset = vReplicatedLogObj.GetOffset(start_index);
    
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

    vTermVoteObj.UpdateCurrentTerm(currentTerm);

    // Replicated log
    auto rl_entries = pReplicatedLogObj.ParseLog();

    for (auto entry : rl_entries)
    {
        vReplicatedLogObj.Append(entry.term, entry.key, entry.value, entry.offset); 
    }
}

// Tester
// g++ state_helper.cc term_vote_volatile.cc term_vote_persistent.cc replicated_log_persistent.cc replicated_log_volatile.cc -Wall -o state
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

    // Test Append
    // obj.Append(1, "key1", "value");
    // dbgprintf("Log length = %d\n", obj.GetLogLength());

    // obj.Append(2, "key2", "value");
    // dbgprintf("Log length = %d\n", obj.GetLogLength());

    // obj.Append(3, "key3", "value");
    // dbgprintf("Log length = %d\n", obj.GetLogLength());

    // Test Insert
    // vector<Entry> entries;
    // entries.push_back(Entry(1, "a", "d"));
    // entries.push_back(Entry(2, "b", "e"));
    // obj.Insert(1, entries);
    // dbgprintf("Log length = %d\n", obj.GetLogLength());

    // Test Get term
    // dbgprintf("Term at index %d is %d\n", 1, obj.GetTermAtIndex(1));
    // dbgprintf("Term at index %d is %d\n", 2, obj.GetTermAtIndex(2));

//     return 0;
// }