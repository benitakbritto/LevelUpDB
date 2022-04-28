#include "levelDBWrapper.h"

/**
 * @brief Get value for given key from DB
 * 
 * @param key : Key for which value is seeked
 * @param value : String value for given key
 * @return Status : Status code for LevelDB Operation
 */
Status LevelDBWrapper::Get(string key, string &value) {
    Status s = db->Get(ReadOptions(), key, &value);
    if (s.ok()) {
        dbgprintf("[INFO]: Key-Value Pair found\n");
    } else {
        cout << "[ERROR]: Read Failed" << s.ToString() << endl;
    }
    return s;
}

/**
 * @brief Set key value pair in DB
 * 
 * @param key : String Key 
 * @param value : String value for given key
 * @return Status : Status code for LevelDB Operation
 */
Status LevelDBWrapper::Put(string key, string value) {
    Status s = db->Put(WriteOptions(), key, value);
    if (s.ok()) {
        dbgprintf("[INFO]: Key-Value Pair Set\n");
    } else {
        cout << "[ERROR]: Write Failed" << s.ToString() << endl;
    }
    return s;
}

/**
 * @brief Recreate state from given snapshot
 * 
 * @param snap_state : Map with snapshot data
 */
void LevelDBWrapper::AtomicPut(unordered_map<string, string> snap_state) {
    std::string value;
    leveldb::WriteBatch batch;

    // Delete outdated snapshots
    LevelDBWrapper::ReleaseSnapshot();
    
    // Set state in a batch operation
    for (auto& it: snap_state) {
        batch.Put(it.first, it.second);
    }
}

/**
 * @brief Create state from given DB state
 * 
 * @return unordered_map<string, string> : Snapshot map
 */
unordered_map<string, string> LevelDBWrapper::GetSnapshot() {
    // Delete outdated snapshot
    LevelDBWrapper::ReleaseSnapshot();

    // Create snapshot instance
    snapshot_options.snapshot = db->GetSnapshot();
    unordered_map<string, string> snap_state;

    leveldb::Iterator* it = db->NewIterator(snapshot_options);
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        snap_state[it->key().ToString()] = it->value().ToString();
    }

    if (it->status().ok()) {
        dbgprintf("[INFO]: Snapshot Created\n");
    } else {
        cout << "[ERROR]: Snapshot Creation Failed" << it->status().ToString() << endl;
    }
    
    delete it;
    return snap_state;
}

/**
 * @brief Deletes existing snapshot
 * 
 */
void LevelDBWrapper::ReleaseSnapshot() {
    if (snapshot_options.snapshot != nullptr) {
        db->ReleaseSnapshot(snapshot_options.snapshot);
        dbgprintf("[INFO]: Snapshot Released\n");
    }
}

/*
*   @brief Uncomment to test LevelDBWrapper
*/
// int main(){
//     LevelDBWrapper levelDBWrapper;
//     Status s = levelDBWrapper.Put("k1","v1");
//     s = levelDBWrapper.Put("k2","v2");
//     s = levelDBWrapper.Put("k3","v3");
//     s = levelDBWrapper.Put("k2","v4");

//     unordered_map<string,string> abc = levelDBWrapper.GetSnapshot();
//     levelDBWrapper.AtomicPut(abc);
//     if (!s.ok()) {
//         throw ("[ERROR]: Cannot test LEVELDB");
//     }
// }