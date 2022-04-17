// TODO: Reader-writer locks
// TODO: Add to cmake
#include "replicated_log_volatile.h"

void VolatileReplicatedLog::Append(int term, string key, string value, int file_offset)
{
    dbgprintf("[DEBUG]: AddEntry - Entered function\n");
    volatile_replicated_log.push_back(LogEntry(term, key, value, file_offset));
    dbgprintf("[DEBUG]: AddEntry - Exited function\n");
}

int VolatileReplicatedLog::GetOffset(int index)
{
    int len = volatile_replicated_log.size();
    if (len <= index)
    {
        throw runtime_error("[ERROR]: Invalid index");
    }

    return volatile_replicated_log[index].file_offset;
}

string VolatileReplicatedLog::GetKey(int index)
{
    int len = volatile_replicated_log.size();
    if (len <= index)
    {
        throw runtime_error("[ERROR]: Invalid index");
    }

    return volatile_replicated_log[index].key;
}

string VolatileReplicatedLog::GetValue(int index)
{
    int len = volatile_replicated_log.size();
    if (len <= index)
    {
        throw runtime_error("[ERROR]: Invalid index");
    }

    return volatile_replicated_log[index].value;
}

int VolatileReplicatedLog::GetTerm(int index)
{
    int len = volatile_replicated_log.size();
    if (len <= index)
    {
        throw runtime_error("[ERROR]: Invalid index");
    }

    return volatile_replicated_log[index].term;
}