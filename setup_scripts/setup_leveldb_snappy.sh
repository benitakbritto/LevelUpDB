#!/bin/sh
BLUE='\033[0;44m'
NOCOLOR='\033[0m'

git clone --recurse-submodules https://github.com/google/leveldb.git
cd leveldb

mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .

cd ~

git clone https://github.com/google/snappy.git
cd snappy

git submodule update --init
mkdir build
cd build && cmake ../ && make