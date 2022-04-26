/*
*   @brief Handles persistent snapshots
*/

#ifndef SNAPSHOT_PERSISTENT_H
#define SNAPSHOT_PERSISTENT_H

#include "common.h"
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <cstring>
#include <vector>
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
        vector<string,string> snapshotObj;
    
    public:
        vector<string,string> GetSnapshot();
        void SetSnapshot(vector<string,string>);
};

#endif
