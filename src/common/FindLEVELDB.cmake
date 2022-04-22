set(LEVELDB_INCLUDE_DIR "/home/benitakbritto/CS739-P4/src/third_party/leveldb/include/")
set(SNAPPY_INCLUDE_DIR "/home/benitakbritto/CS739-P4/src/third_party/snappy/")

FIND_LIBRARY(
    SNAPPY_LIBRARY 
    NAMES libsnappy.a 
    PATHS /lib64 /lib /usr/lib64 /usr/lib /usr/local/lib64 /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/local/include /home/benitakbritto/CS739-P4/src/third_party/snappy/build
    NO_CACHE
)

FIND_LIBRARY(
    LEVELDB_LIBRARY 
    NAMES libleveldb.a 
    PATHS /lib64 /lib /usr/lib64 /usr/lib /usr/local/lib64 /usr/local/lib /usr/lib/x86_64-linux-gnu /home/benitakbritto/CS739-P4/src/third_party/leveldb/build
    NO_CACHE
)
