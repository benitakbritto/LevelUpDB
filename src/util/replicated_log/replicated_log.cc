// TODO: Add to cmake

#include "replicated_log.h"

/*
*   @brief Add log entry to the end
*
*   @param term
*   @param key
*   @param value
*/
void ReplicatedLogHelper::Append(int term, string key, string value)
{
    dbgprintf("[DEBUG]: Append - Entering function\n");
    int offset = 0;

    offset = pObj.GetEndOfFileOffset();
    pObj.Append(term, key, value, offset);
    vObj.Append(term, key, value, offset);

    dbgprintf("[DEBUG]: Append - Exiting function\n");
}

/*
*   @brief Add log entry(ies) from the specified index
*
*   @param start index
*   @param entries (term, key, value
*/
void ReplicatedLogHelper::Insert(int start_index, vector<Entry> &entries)
{
    dbgprintf("[DEBUG]: Insert - Entering function\n");
    int offset = 0;
    int index = 0;

    index = start_index;
    offset = vObj.GetOffset(start_index);
    
    // preserve log up to offset bytes
    if (truncate(REPLICATED_LOG_PATH, offset) == -1)
    {
        throw runtime_error("[ERROR]: truncate failed\n");
    }

    for (auto val : entries)
    {
        dbgprintf("[DEBUG]: Insert - offset = %d\n", offset);

        pObj.Insert(offset, val.term, val.key, val.value);
        vObj.Insert(index, val.term, val.key, val.value, offset);
        
        offset = pObj.GetCurrentFileOffset();
        index += 1;
    }

    dbgprintf("[DEBUG]: Insert - Exiting function\n");
}

/*
*   @brief Get the number of total entries in the log
*/
int ReplicatedLogHelper::GetLogLength()
{
    return vObj.GetLength();
}

/*
*   @brief Get the term number at the specified index
*
*   @param index
*/
int ReplicatedLogHelper::GetTermAtIndex(int index)
{
    return vObj.GetTerm(index);
}

// TODO
void ReplicatedLogHelper::Init()
{
    auto entries = pObj.ParseLog();

    for (auto entry : entries)
    {
        vObj.Append(entry.term, entry.key, entry.value, entry.offset); // TODO: Offsets
    }

    // remove later
    vObj.PrintVolatileLog();
}

/*
*   @brief Tester - Uncomment to test individually
*   @usage g++ replicated_log.cc replicated_log_persistent.cc replicated_log_volatile.cc -Wall -o log && ./log
*/
// int main()
// {
//     ReplicatedLogHelper obj;

//     // // Test Append
//     // obj.Append(1, "key1", "value");
//     // dbgprintf("Log length = %d\n", obj.GetLogLength());

//     // obj.Append(2, "key2", "value");
//     // dbgprintf("Log length = %d\n", obj.GetLogLength());

//     // obj.Append(3, "key3", "value");
//     // dbgprintf("Log length = %d\n", obj.GetLogLength());

//     // // Test Insert
//     // vector<Entry> entries;
//     // entries.push_back(Entry(1, "a", "d"));
//     // entries.push_back(Entry(2, "b", "e"));
//     // obj.Insert(1, entries);
//     // dbgprintf("Log length = %d\n", obj.GetLogLength());

//     // // Test Get term
//     // dbgprintf("Term at index %d is %d\n", 1, obj.GetTermAtIndex(1));
//     // dbgprintf("Term at index %d is %d\n", 2, obj.GetTermAtIndex(2));

//     // Test Init
//     obj.Init();


//     return 0;
// }