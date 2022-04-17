#ifndef PERSISTENT_REPLICATED_LOG_H
#define PERSISTENT_REPLICATED_LOG_H

#include "common.h"
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <cstring>

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
    string CreateLogEntry(int term, string key, string value);
    void SetEndOfFileOffset(int offset);
    void GoToOffset(int offset);
    
    void GoToEndOfFile();
    void WriteToLog(int term, string key, string value);
    int _endOfFileOffset = 0;

public:
    


    PersistentReplicatedLog();
    void Insert(int term, string key, string value);
    void Append(int term, string key, string value);
    int GetEndOfFileOffset();
};


#endif
