#ifndef LEVELDBWRAPPER_H
#define LEVELDBWRAPPER_H

#include <cassert>
#include <iostream>
#include "leveldb/db.h"

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
    
    public:
    LevelDBWrapper(){
        options.create_if_missing = true;
        Status status = DB::Open(options, "/tmp/testdb", &db);
        assert(status.ok());
        write_options.sync = true;
    }
   
   Status Get(string key, string &value);
   Status Put(string key, string value);
};

#endif