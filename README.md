# CS-739-P4: LevelUpDB - A Cloud-Native version of LevelDB

## Rubric
<TODO>

## Setup
### gRPC Installation
Follow these steps to install gRPC lib using cmake: https://grpc.io/docs/languages/cpp/quickstart/#setup. 
:warning: make sure to limit the processes by passing number(e.g. 4) during `make -j` command.

for example, instead of `make -j` use `make -j 4`

### LevelDB Installation
Clone and follow the commands mentioned [here](https://github.com/google/leveldb#getting-the-source) and [here](https://github.com/google/leveldb#building) 
  
### Snappy Installation
Clone and follow the commands mentioned [here](https://github.com/google/snappy).
 
### Folder structure
|_src  
|____third_party  
|_______________leveldb  
|_______________snappy  
|_storage  
  
### Build
#### Main source code
0. `cd src/`
1. `chmod 755 build.sh`
2. `./build.sh`

### Run
```
./loadbalancer <ip with port for client to connect> <ip with port for servers to send heartbeat>
./server <my kv ip with port> <my raft ip with port> <lb ip with port>
./keyvalue_client <ip of lb with port>
```

To run Test
```
cd test
chmod 777 build.sh
chmod 777 clean.sh
chmod run_read_test.sh
chmod run_write_test.sh
./build
./run_read_test
./run_write_test
```
Results will be redirected to /results
Note: The `run_read_test` and `run_write_test` assume that the lb kv ip is `0.0.0.0:50051`.


## Assumptions
0. No network partitions : We can safely use matchIndex as the prevLogIndex, no follower would have a commitIndex > leaderCommitIndex
1. Acc to Raft paper, If leaderCommit > commitIndex, set commitIndex = min(leaderCommit, index of last new entry): We set it to leaderCommitIndex because of point 0
2. We are using AssertLeadership RPC to tell the Load Balancer that the leader has changed (Raft does not do this).

## Deliverables
<TODO>
