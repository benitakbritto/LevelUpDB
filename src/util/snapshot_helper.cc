#include "snapshot_helper.h"

int g_snapshot_length = 0;

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
 * @param ip : ip of server
 */
void SnapshotHelper::SetSnapshot(unordered_map<string, string> snapshot, string ip)
{
    vSnapshotObj.SetSnapshot(snapshot);
    pSnapshotObj.SetSnapshot(snapshot, ip);
}

/**
 * @brief Get snapshot length
 * 
 * @return snapshot length
 */
int SnapshotHelper::GetSnapshotLength()
{
    return g_snapshot_length;
}

/**
 * @brief Truncate the log
 * 
 */
void SnapshotHelper::truncateLog()
{
    pSnapshotObj.truncateLog();
}