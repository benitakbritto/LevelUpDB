/*
*   @brief Handles persistent snapshots
*/

#ifndef SNAPSHOT_VOLATILE_H
#define SNAPSHOT_VOLATILE_H

#include "common.h"
#include <cstring>
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
class VolatileSnapshot 
{
    private:
       unordered_map<string, string> snapshotObj;
    
    public:
        unordered_map<string,string> GetSnapshot();
        void SetSnapshot(unordered_map<string,string>);
};

#endif
