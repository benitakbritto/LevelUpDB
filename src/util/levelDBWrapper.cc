#include "levelDBWrapper.h"

Status LevelDBWrapper::Get(string key, string &value) {
    return db->Get(ReadOptions(), key, &value);
}

Status LevelDBWrapper::Put(string key, string value) {
   return db->Put(leveldb::WriteOptions(), key, value);
}

//Test
int main(){
    LevelDBWrapper levelDBWrapper;
    // Status s = levelDBWrapper.Put("k1","v1");
    // if (!s.ok()) cerr << s.ToString() << endl;
}