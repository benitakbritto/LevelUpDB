/*
*   @brief Handles persistent snapshots
*/

#ifndef SNAPSHOT_PERSISTENT_H
#define SNAPSHOT_PERSISTENT_H

#include "common.h"
#include <cstring>
#include <fcntl.h>
#include <unordered_map>
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
class PersistentSnapshot 
{
    private:
       int writeSnapshotToFile(string snapshotFilePath);

    public:
        void SetSnapshot(unordered_map<string,string>, string ip);
        void truncateLog();
};

#endif
