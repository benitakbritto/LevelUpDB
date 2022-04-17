#ifndef PERSISTENT_REPLICATED_LOG_H
#define PERSISTENT_REPLICATED_LOG_H

#include "common.h"
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <cstring>
#include "replicated_log_volatile.h"

/******************************************************************************
 * GLOBALS
 *****************************************************************************/


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
    int endOfFileOffset = 0;
    string CreateLogEntry(int term, string key, string value);
    void SetEndOfFileOffset(int offset);
    void GoToOffset(int offset);
    int GetEndOfFileOffset();
    int GoToEndOfFile();
    VolatileReplicatedLog volatileReplicatedLogObj;

public:
    PersistentReplicatedLog();
    void Insert(int term, string key, string value);
    void Append(int term, string key, string value);
};


#endif
