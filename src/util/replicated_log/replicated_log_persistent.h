#ifndef PERSISTENT_REPLICATED_LOG_H
#define PERSISTENT_REPLICATED_LOG_H

#include "common.h"
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <vector>

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
struct PersistentLogEntry
{
    int term;
    string key;
    string value;
    int offset;
    PersistentLogEntry(int term, string key, string value, int offset) :
                        term(term),
                        key(key),
                        value(value),
                        offset(offset)
                        {}
};

typedef PersistentLogEntry PLogEntry;

/******************************************************************************
 * MACROS
 *****************************************************************************/
#define DELIM                       ","

/******************************************************************************
 * NAMESPACES
 *****************************************************************************/



/******************************************************************************
 * DECLARATION
 *****************************************************************************/
class PersistentReplicatedLog
{
private:
    int fd;
    string CreateLogEntry(int term, string key, string value, int offset);
    
    void GoToOffset(int offset);
    void GoToEndOfFile();
    void WriteToLog(int term, string key, string value, int offset);

public:
    PersistentReplicatedLog();
    
    void Insert(int offset, int term, string key, string value);
    void Append(int term, string key, string value, int offset);
    int GetEndOfFileOffset();
    int GetCurrentFileOffset();
    vector<PLogEntry> ParseLog();
};


#endif
