set(LEVELDB_INCLUDE_DIR "/home/benitakbritto/CS739-P4/src/third_party/leveldb/include/")
set(LEVELDB_LIBRARY "/home/benitakbritto/CS739-P4/src/third_party/leveldb")

#FIND_PATH( LEVELDB_INCLUDE_DIR db.h
#    /home/benitakbritto/CS739-P4/src/third_party/leveldb
#    /usr/local/include
#    /usr/include
#)

FIND_LIBRARY(
    LEVELDB_LIBRARY 
    NAMES libleveldb.a 
    PATHS /lib64 /lib /usr/lib64 /usr/lib /usr/local/lib64 /usr/local/lib /usr/lib/x86_64-linux-gnu /home/benitakbritto/CS739-P4/src/third_party/leveldb/build/lib/
    NO_CACHE
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args("LEVELDB" DEFAULT_MSG
    LEVELDB_INCLUDE_DIR
    LEVELDB_LIBRARY
)