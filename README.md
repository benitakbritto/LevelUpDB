# CS-739-P4: LevelUpDB - A Cloud-Native version of LevelDB

## Locating the code
### Server
Our server code includes a key value server, communicates with the loaf balancer through heartbeats and implements Raft. 
Source code: `src/server/*`
1. `class KeyValueOpsServiceImpl`: responds to client's `Get()`, `Put()` requests
2. `class LBNodeCommClient`: communicates with the load balancer for hearbeats and leader asserts their leadership 
3. `class RaftServer`: implements `RequestVote()` and `AppendEntries()`

### Client
Our client side implementation library resides here. 
Source code : `src/client/*`
1. `class KeyValueClient`: has the client implementation for making `Get()` and `Put()` calls that is shared by the user and the load balancer
2. `class KeyValueService`: has the implementation for the load balancer

### Test Code
Automates our test cases for `Get()` and `Put()`. 
Source code: `test/`
1. `read.cc`: Tests `Get()`
2. `write.cc`: Tests `Put`
3. `run_read_script.sh`: Runs the tests for `Get()`
4. `run_write_script.sh`: Runs the tests for `Put()`

### Utilities
Source code: `src/util/*`
1. LevelDB Wrapper code: `levelDBWrapper.cc`
2. Locks code: `locks.cc`
3. Replicated Log Helper code : `replicated_log_persistent.cc` and `replicated_log_volatile.cc`
4. Voting state Helper code: `term_vote_persistent.cc` and `term_vote_volatile.cc` 
5. Volatile state code: `state_volatile.cc` 
6. State Helper code: `state_helper.cc`

Note: For the persistent states of Raft, we also implemented volatile versions for fast lookups.
The persistent state is stored in the storage/ folder.  

## Setup
### Folder structure
|_src  
|____third_party  
|_______________leveldb  
|_______________snappy  
|_storage 

### gRPC Installation
Follow these steps to install gRPC lib using cmake [here] (https://grpc.io/docs/languages/cpp/quickstart/#setup) 
:warning: make sure to limit the processes by passing number(e.g. 4) during `make -j` command.

for example, instead of `make -j` use `make -j 4`

### LevelDB Installation
Clone and follow the commands mentioned [here](https://github.com/google/leveldb#getting-the-source) and [here](https://github.com/google/leveldb#building) 
  
### Snappy Installation
Clone and follow the commands mentioned [here](https://github.com/google/snappy).
  
## Build
### Building src
0. `cd src/`
1. `chmod 755 build.sh`
2. `chmod 755 clean.sh`
3. `./clean.sh`
4. `./build.sh`

### Building test
0. `cd test/`
1. `chmod 755 build.sh`
2. `chmod 755 clean.sh`
3. `./clean.sh`
4. `./build.sh`

## Run
### Running src
We need to start the loadbalancer and server
```
cd src/cmake/build
./loadbalancer <ip with port for client to connect> <ip with port for servers to send heartbeat>
./server <my kv ip with port> <my raft ip with port> <lb ip with port>
```

### Running test
To run read (calls get API) test
```
cd test
chmod run_read_test.sh
chmod run_write_test.sh
./run_read_test
```

To run write (calls put API) test
```
cd test
chmod run_read_test.sh
chmod run_write_test.sh
./run_write_test
```
Results will be redirected to /results
Note: The `run_read_test` and `run_write_test` assume that the loadbalancer kv ip is `0.0.0.0:50051`.

## Deliverables
1. Presentation [here] (https://uwprod-my.sharepoint.com/:p:/g/personal/bbritto_wisc_edu/EYl_rcnRnodAlgncz1v7rEoBUMUmoI-uvaQy9J87yX4h6g?e=pECxqO)
2. Demos [here] (https://drive.google.com/drive/folders/14HGs8kPEDF6nVxDD0LJdAZ9ipBM-Y0nk?usp=sharing)
3. Report [here] (https://docs.google.com/document/d/1XYwwFvuJtgp1EpDObnG--ibQ3sdL7XUStoMfCV0jiUY/edit?usp=sharing)
