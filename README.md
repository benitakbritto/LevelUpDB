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


## Assumptions
0. No network partitions : We can safely use matchIndex as the prevLogIndex, no follower would have a commitIndex > leaderCommitIndex
1. Acc to Raft paper, If leaderCommit > commitIndex, set commitIndex = min(leaderCommit, index of last new entry): We set it to leaderCommitIndex because of point 0
2. We are using AssertLeadership RPC to tell the Load Balancer that the leader has changed (Raft does not do this).

## Deliverables
<TODO>
