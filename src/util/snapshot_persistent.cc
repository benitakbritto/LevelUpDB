#include "snapshot_persistent.h"

/*
*   @brief get snapshot
*
*   @return snapshot
*/
vector<string, string> PersistentSnapshot::GetSnapshot()
{
    return snapshotObj;
}

/*
*   @brief Set snapshot
*/
void PersistentSnapshot::SetSnapshot(vector<string, string> snapshot)
{
    snapshotObj = snapshot;
}
