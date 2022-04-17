#ifndef VOLATILE_REPLICATED_LOG_H
#define VOLATILE_REPLICATED_LOG_H

#include "common.h"
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <cstring>
#include <vector>

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
struct ReplicatedLogEntry
{
    int term;
    string key;
    string value;
    int file_offset;
    ReplicatedLogEntry(int term,
                    string key,
                    string value,
                    int file_offset)
                    : 
                    term(term),
                    key(key),
                    value(value),
                    file_offset(file_offset)
                    {}                      
};

typedef ReplicatedLogEntry LogEntry;

/******************************************************************************
 * MACROS
 *****************************************************************************/


/******************************************************************************
 * NAMESPACES
 *****************************************************************************/



/******************************************************************************
 * DECLARATION
 *****************************************************************************/
class VolatileReplicatedLog
{
private:
    vector<LogEntry> volatile_replicated_log;

public:
    VolatileReplicatedLog() {}
    void Append(int term, string key, string value, int file_offset);
    int GetOffset(int index);
    int GetTerm(int index);
    string GetKey(int index);
    string GetValue(int index);
};


#endif
