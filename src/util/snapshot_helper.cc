#include "snapshot_helper.h"

/**
 * @brief Return Volatile Snapshot (Map)
 * 
 * @return unordered_map<string, string> Snapshot in-memory state 
 */
unordered_map<string, string> SnapshotHelper::GetSnapshot()
{
    return vSnapshotObj.GetSnapshot();
}

/**
 * @brief Set snapshot (Map) state in-memory and disk
 * 
 * @param snapshot : Snapshot state (Map)
 */
void SnapshotHelper::SetSnapshot(unordered_map<string, string> snapshot)
{
    vSnapshotObj.SetSnapshot(snapshot);
    pSnapshotObj.SetSnapshot(snapshot);
}