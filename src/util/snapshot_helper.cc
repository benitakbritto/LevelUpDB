#include "snapshot_helper.h"

unordered_map<string, string> SnapshotHelper::GetSnapshot()
{
    return vSnapshotObj.GetSnapshot();
}

void SnapshotHelper::GetSnapshot(unordered_map<string, string> snapshot)
{
    vSnapshotObj.SetSnapshot(snapshot);
    pSnapshotObjj.SetSnapshot(snapshot);
}