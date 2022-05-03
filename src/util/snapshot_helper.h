/*
*   @brief Handles volatile and persistent snapshots
*/

#ifndef SNAPSHOT_HELPER_H
#define SNAPSHOT_HELPER_H

#include "common.h"
#include <cstring>
#include <unordered_map>
#include "snapshot_volatile.h"
#include "snapshot_persistent.h"
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
class SnapshotHelper
{
    private:
        PersistentSnapshot pSnapshotObj;
        VolatileSnapshot vSnapshotObj;
    
    public:
        unordered_map<string, string> GetSnapshot();
        void SetSnapshot(unordered_map<string, string> snapshot, string ip);
        int GetSnapshotLength();
        void truncateLog();
};


#endif
