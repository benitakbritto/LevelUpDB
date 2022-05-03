#include "snapshot_persistent.h"
#include "common.h"
#include <sys/types.h>
#include <unistd.h>

/**
 * @brief Set snapshot and flush to storage, truncate current log
 * 
 * @param snapshot : Snapshot state (Map)
 * @param ip : ip of server
 */
void PersistentSnapshot::SetSnapshot(unordered_map<string, string> snapshot, string ip)
{

    string snapshotFilePath = SNAPSHOT_PATH + ip;
    int fd = open(snapshotFilePath.c_str(), O_WRONLY | O_CREAT, 0666);

    if (fd == -1) 
    {
       	cout << errno << endl;
	    throw runtime_error("[ERROR]: Could not create snapshot file");
    }
    writeSnapshotToFile(snapshotFilePath);
    truncateLog();
}

/**
 * @brief Write snapshot obj to file
 * 
 * @param snapshotFilePath description
 * 
 * @return error code
 */
int PersistentSnapshot::writeSnapshotToFile(string snapshotFilePath)
{
    // TODO: Parse snapshot and write to file
    return 0;
}


/**
 * @brief Truncate current log file
 * 
 */
void PersistentSnapshot::truncateLog() 
{

}