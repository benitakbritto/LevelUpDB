#include "snapshot_helper.h"

unordered_map<string, string> SnapshotHelper::GetSnapshot()
{
    return vSnapshotObj.GetSnapshot();
}

void SnapshotHelper::SetSnapshot(unordered_map<string, string> snapshot)
{
    vSnapshotObj.SetSnapshot(snapshot);
    pSnapshotObj.SetSnapshot(snapshot);
}