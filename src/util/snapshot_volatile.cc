#include "snapshot_volatile.h"

/*
*   @brief get snapshot
*
*   @return snapshot
*/
unordered_map<string, string> VolatileSnapshot::GetSnapshot()
{
    return snapshotObj;
}

/*
*   @brief Set snapshot
*/
void VolatileSnapshot::SetSnapshot(unordered_map<string, string> snapshot)
{
    snapshotObj = snapshot;
}
