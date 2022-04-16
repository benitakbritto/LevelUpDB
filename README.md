# CS-739-P4

## Replication Strategy

## Durability

## Setup
### gRPC Installation
Follow these steps to install gRPC lib using cmake: https://grpc.io/docs/languages/cpp/quickstart/#setup. 
:warning: make sure to limit the processes by passing number(e.g. 4) during `make -j` command.

for example, instead of `make -j` use `make -j 4`

### Build
#### Main source code
0. cd src/
1. chmod 755 build.sh
2. ./build.sh

#### Performance scripts
0. cd performance/
1. chmod 755 build.sh
2. ./build.sh

### Run
#### Client
```
cd src/cmake/build
./blockstorage_client
```

OR

```
cd performance/
./build
cd cmake/build
<ANY OF THE EXECUTABLES LISTED IN THIS DIRECTORY>
```

#### Load Balancer
```
./load_balancer
```

#### Primary Server
```
./blockstorage_server PRIMARY [self_addr_lb] [self_addr_peer] [peer_addr] [lb_addr]
```
Eg.
```
./blockstorage_server PRIMARY 20.127.48.216:40051 0.0.0.0:60052 20.127.55.97:60053 20.228.235.42:50056
```

#### Backup Server
```
./blockstorage_server BACKUP [self_addr_lb] [self_addr_peer] [peer_addr] [lb_addr]
```
Eg.
```
./blockstorage_server BACKUP 20.127.55.97:40051 0.0.0.0:60053 20.127.48.216:60052 20.228.235.42:50056
```

#### Performance Scripts

## Deliverables
