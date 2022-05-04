#include "levelDBWrapper.h"

Status LevelDBWrapper::Get(string key, string &value) {
    Status s = db->Get(ReadOptions(), key, &value);
    if (s.ok()) {
        dbgprintf("[INFO]: Key-Value Pair found\n");
    } else {
        cout << "[ERROR]: Read Failed : " << s.ToString() << endl;
    }
    return s;
}

Status LevelDBWrapper::Put(string key, string value) {
    Status s = db->Put(WriteOptions(), key, value);
    if (s.ok()) {
        dbgprintf("[INFO]: Key-Value Pair found\n");
    } else {
        cout << "[ERROR]: Write Failed : " << s.ToString() << endl;
    }
    return s;
}

/*
*   @brief Uncomment to test LevelDBWrapper
*/
// int main(){
    // LevelDBWrapper levelDBWrapper;
    // Status s = levelDBWrapper.Put("k1","v1");
    // if (!s.ok()) {
    //     throw ("[ERROR]: Cannot test LEVELDB");
    // }
// }