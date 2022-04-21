# CS-739-P4: LevelUpDB - A Cloud-Native version of LevelDB

## Rubric
<TODO>

## Setup
### gRPC Installation
Follow these steps to install gRPC lib using cmake: https://grpc.io/docs/languages/cpp/quickstart/#setup. 
:warning: make sure to limit the processes by passing number(e.g. 4) during `make -j` command.

for example, instead of `make -j` use `make -j 4`

 ### LevelDB Installation
Follow the commands mentioned [here](https://github.com/google/leveldb#getting-the-source) and [here](https://github.com/google/leveldb#building) 
  
  
### Build
#### Main source code
0. `cd src/`
1. `chmod 755 build.sh`
2. `./build.sh`

### Run
```
./loadbalancer
./server <server addr>
./keyvalue_client
```

## Deliverables
<TODO>
