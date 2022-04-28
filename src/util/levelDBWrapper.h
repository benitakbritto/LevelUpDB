#ifndef LEVELDBWRAPPER_H
#define LEVELDBWRAPPER_H

#include "common.h"
#include <thread>
#include <unistd.h>
#include <cassert>
#include <iostream>
#include <unordered_map>
#include "leveldb/db.h"
#include "leveldb/write_batch.h"

/******************************************************************************
 * NAMESPACES
 *****************************************************************************/
using namespace std;
using namespace leveldb;

/******************************************************************************
 * DECLARATION
 *****************************************************************************/

class LevelDBWrapper {
    DB* db;
    Options options;
    WriteOptions write_options;
    ReadOptions read_options;
    ReadOptions snapshot_options;
    
    public:
    LevelDBWrapper(){
        options.create_if_missing = true;
        leveldb::Status status = DB::Open(options, "/tmp/testdb"+to_string(getpid()), &db);
        assert(status.ok());
        write_options.sync = true;
    }

    ~LevelDBWrapper(){
        delete db;
    }
   
   leveldb::Status Get(string key, string &value);
   leveldb::Status Put(string key, string value);
   void AtomicPut(unordered_map<string, string>);
   unordered_map<string, string> GetSnapshot();
   void ReleaseSnapshot();
};

#endif