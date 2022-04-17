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
struct ReplicatedLogEntry
{
    int term;
    string key;
    string value;
    ReplicatedLogEntry(int term, string key, string value)
            : term(term), key(key), value(value) {}
};

typedef ReplicatedLogEntry Entry;

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
    void Insert(int start_index, vector<Entry> &entries);
};


#endif
