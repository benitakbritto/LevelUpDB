// TODO: Reader-writer locks
// TODO: Add to cmake
#include "replicated_log_volatile.h"

/*
*   @brief Add command entry to the end of the in-mem log
*
*   @param term 
*   @param key 
*   @param value 
*   @param file_offset 
*/
void VolatileReplicatedLog::Append(int term, string key, string value, int file_offset)
{
    dbgprintf("[DEBUG]: Append - Entered function\n");

    volatile_replicated_log.push_back(LogEntry(term, key, value, file_offset));
    
    dbgprintf("[DEBUG]: Append - Exited function\n");
}

/*
*   @brief Add command entry to the specified index of the in-mem log
*
*   @param index 
*   @param term 
*   @param key 
*   @param value 
*   @param file_offset 
*/
void VolatileReplicatedLog::Insert(int index, int term, string key, string value, int file_offset)
{
    dbgprintf("[DEBUG]: Insert - Entering function\n");
    dbgprintf("[DEBUG]: Insert - index = %d\n", index);
    int len = GetLength();
    if (index > len)
    {
        throw runtime_error("[ERROR]: Invalid index");
    }
    else if (index == len)
    {
        volatile_replicated_log.push_back(LogEntry(term, key, value, file_offset));
    }
    else
    {
        volatile_replicated_log[index] = LogEntry(term, key, value, file_offset);
    }

    dbgprintf("[DEBUG]: Insert - Exiting function\n");
}

/*
*   @brief Get offset of specified index
*
*   @param index
*   @return offset 
*/
int VolatileReplicatedLog::GetOffset(int index)
{
    return isValidIndex(index) ? volatile_replicated_log[index].file_offset : -1;
}

/*
*   @brief Get key of specified index
*
*   @param index
*   @return key 
*/
string VolatileReplicatedLog::GetKey(int index)
{
    return isValidIndex(index) ? volatile_replicated_log[index].key : string("");
}

/*
*   @brief Get value of specified index
*
*   @param index
*   @return value 
*/
string VolatileReplicatedLog::GetValue(int index)
{
    return isValidIndex(index) ? volatile_replicated_log[index].value : string("");
}

/*
*   @brief Get term of specified index
*
*   @param index
*   @return term 
*/
int VolatileReplicatedLog::GetTerm(int index)
{
    return isValidIndex(index) ? volatile_replicated_log[index].term : 0; 
}

/*
*   @brief Check if specified index is valid
*
*   @param index
*   @return validity 
*/
bool VolatileReplicatedLog::isValidIndex(int index)
{
    int len = GetLength();
    if (index < 0 || len <= index)
    {
        // throw runtime_error("[ERROR]: Invalid index");
        return false;
    }

    return true;
}

/*
*   @brief Get total entries in log
*/
int VolatileReplicatedLog::GetLength()
{
    return volatile_replicated_log.size();
}

/*
*   @brief Prints all the contents in the volatile log 
*/
void VolatileReplicatedLog::PrintVolatileLog()
{
    for (auto entry : volatile_replicated_log)
    {
        dbgprintf("%d | %s | %s | %d\n", entry.term, entry.key.c_str(), entry.value.c_str(), entry.file_offset);
    }
}