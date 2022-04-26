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
        PersistentSnapshot pSnapshotObjj;
        VolatileSnapshot vSnapshotObj;
    
    public:
        unordered_map<string, string> GetSnapshot();
        void GetSnapshot(unordered_map<string, string> snapshot);
};


#endif
