#!/bin/sh
# Note: Make sure to run ./build.sh script before 
BLUE='\033[0;44m'
NOCOLOR='\033[0m'

# Change dir
cd cmake/build

# Test: single read
echo "${BLUE} Starting Single Read Tests${NOCOLOR}"
# strong consistency
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 0 -l 0 -q 0 -w 0 > ../../../results/read/single/1
# eventual consistency (quorums of size 1-3)
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 0 -l 1 -q 1 -w 0 > ../../../results/read/single/2
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 0 -l 1 -q 2 -w 0 > ../../../results/read/single/3
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 0 -l 1 -q 3 -w 0 > ../../../results/read/single/4



# Test: concurrent read same key
echo "${BLUE} Starting Concurrent Read - Same Key Tests${NOCOLOR}"
# 2 Workers
# strong consistency
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 0 -q 0 -w 2 > ../../../results/read/concurrent_same/2_1
# eventual consistency (quorums of size 1-3)
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 1 -q 1 -w 2 > ../../../results/read/concurrent_same/2_2
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 1 -q 2 -w 2 > ../../../results/read/concurrent_same/2_3
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 1 -q 3 -w 2 > ../../../results/read/concurrent_same/2_4

# 4 Workers
# strong consistency
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 0 -q 0 -w 4  > ../../../results/read/concurrent_same/4_1
# eventual consistency (quorums of size 1-3)
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 1 -q 1 -w 4 > ../../../results/read/concurrent_same/4_2
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 1 -q 2 -w 4 > ../../../results/read/concurrent_same/4_3
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 1 -q 3 -w 4 > ../../../results/read/concurrent_same/4_4

# 8 Workers
# strong consistency
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 0 -q 0 -w 8 > ../../../results/read/concurrent_same/8_1
# eventual consistency (quorums of size 1-3)
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 1 -q 1 -w 8 > ../../../results/read/concurrent_same/8_2
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 1 -q 2 -w 8 > ../../../results/read/concurrent_same/8_3
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 1 -q 3 -w 8 > ../../../results/read/concurrent_same/8_4

# 16 Workers
# strong consistency
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 0 -q 0 -w 16 > ../../../results/read/concurrent_same/16_1
# eventual consistency (quorums of size 1-3)
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 1 -q 1 -w 16 > ../../../results/read/concurrent_same/16_2
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 1 -q 2 -w 16 > ../../../results/read/concurrent_same/16_3
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 1 -q 3 -w 16 > ../../../results/read/concurrent_same/16_4

# 32 Workers
# strong consistency
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 0 -q 0 -w 32 > ../../../results/read/concurrent_same/32_1
# eventual consistency (quorums of size 1-3)
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 1 -q 1 -w 32 > ../../../results/read/concurrent_same/32_2
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 1 -q 2 -w 32 > ../../../results/read/concurrent_same/32_3
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 1 -q 3 -w 32 > ../../../results/read/concurrent_same/32_4



# Test: concurrent read diff key
echo "${BLUE} Starting Concurrent Read - Different Key Tests${NOCOLOR}"
# 2 Workers
# strong consistency
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 0 -q 0 -w 2 > ../../../results/read/concurrent_diff/2_1
# eventual consistency (quorums of size 1-3)
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 1 -q 1 -w 2 > ../../../results/read/concurrent_diff/2_2
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 1 -q 2 -w 2 > ../../../results/read/concurrent_diff/2_3
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -l 1 -q 3 -w 2 > ../../../results/read/concurrent_diff/2_4

# 4 Workers
# strong consistency
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -l 0 -q 0 -w 4 > ../../../results/read/concurrent_diff/4_1
# eventual consistency (quorums of size 1-3)
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -l 1 -q 1 -w 4 > ../../../results/read/concurrent_diff/4_2
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -l 1 -q 2 -w 4 > ../../../results/read/concurrent_diff/4_3
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -l 1 -q 3 -w 4 > ../../../results/read/concurrent_diff/4_4

# 8 Workers
# strong consistency
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -l 0 -q 0 -w 8 > ../../../results/read/concurrent_diff/8_1
# eventual consistency (quorums of size 1-3)
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -l 1 -q 1 -w 8 > ../../../results/read/concurrent_diff/8_2
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -l 1 -q 2 -w 8 > ../../../results/read/concurrent_diff/8_3
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -l 1 -q 3 -w 8 > ../../../results/read/concurrent_diff/8_4

# 16 Workers
# strong consistency
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -l 0 -q 0 -w 16 > ../../../results/read/concurrent_diff/16_1
# eventual consistency (quorums of size 1-3)
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -l 1 -q 1 -w 16 > ../../../results/read/concurrent_diff/16_2
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -l 1 -q 2 -w 16 > ../../../results/read/concurrent_diff/16_3
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -l 1 -q 3 -w 16 > ../../../results/read/concurrent_diff/16_4

# 32 Workers
# strong consistency
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -l 0 -q 0 -w 32 > ../../../results/read/concurrent_diff/32_1
# eventual consistency (quorums of size 1-3)
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -l 1 -q 1 -w 32 > ../../../results/read/concurrent_diff/32_2
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -l 1 -q 2 -w 32 > ../../../results/read/concurrent_diff/32_3
./read 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -l 1 -q 3 -w 32 > ../../../results/read/concurrent_diff/32_4