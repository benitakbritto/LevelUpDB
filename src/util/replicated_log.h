#ifndef REPLICATED_LOG_H
#define REPLICATED_LOG_H

#include "common.h"
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include "replicated_log_persistent.h"
#include "replicated_log_volatile.h"

/******************************************************************************
 * GLOBALS
 *****************************************************************************/


/******************************************************************************
 * MACROS
 *****************************************************************************/


/******************************************************************************
 * NAMESPACES
 *****************************************************************************/



/******************************************************************************
 * DECLARATION
 *****************************************************************************/
class ReplicatedLogHelper
{
private:
    PersistentReplicatedLog pObj;
    VolatileReplicatedLog vObj;
    
public:
    ReplicatedLogHelper() {}
    void Append(int term, string key, string value);
    void Insert(int index, int term, string key, string value);
};


#endif
