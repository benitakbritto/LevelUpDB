#ifndef REPLICATED_LOG_H
#define REPLICATED_LOG_H

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
class ReplicatedLog
{
private:
    int fd;
    string CreateLogEntry(int term, string key, string value);


public:
    ReplicatedLog();

    void Insert(int term, string key, string value);
    void Append(int term, string key, string value);



};



#endif
