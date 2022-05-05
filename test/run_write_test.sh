#!/bin/sh
# Note: Make sure to run ./build.sh script before 
BLUE='\033[0;44m'
NOCOLOR='\033[0m'

# Change dir
cd cmake/build

# Test: single write
echo "${BLUE} Starting Singe Write Tests${NOCOLOR}"
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 0 -s 64 -w 0 > ../../../results/write/single/1
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 0 -s 256 -w 0 > ../../../results/write/single/2
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 0 -s 1024 -w 0 > ../../../results/write/single/3
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 0 -s 4096 -w 0 > ../../../results/write/single/4
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 0 -s 16384 -w 0 > ../../../results/write/single/5


# Test: concurrent write - same key
echo "${BLUE} Starting Concurrent Write - Same Key Tests${NOCOLOR}"
# 2 workers
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 64 -w 2 > ../../../results/write/concurrent_same/2_1
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 256 -w 2 > ../../../results/write/concurrent_same/2_2
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 1024 -w 2 > ../../../results/write/concurrent_same/2_3
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 4098 -w 2 > ../../../results/write/concurrent_same/2_4 
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 16384 -w 2 > ../../../results/write/concurrent_same/2_5

# 4 workers
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 64 -w 4 > ../../../results/write/concurrent_same/4_1
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 256 -w 4 > ../../../results/write/concurrent_same/4_2
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 1024 -w 4 > ../../../results/write/concurrent_same/4_3
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 4096 -w 4 > ../../../results/write/concurrent_same/4_4
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 16384 -w 4 > ../../../results/write/concurrent_same/4_5

# 8 workers
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 64 -w 8 > ../../../results/write/concurrent_same/8_1
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 256 -w 8 > ../../../results/write/concurrent_same/8_2
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 1024 -w 8 > ../../../results/write/concurrent_same/8_3
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 4096 -w 8 > ../../../results/write/concurrent_same/8_4
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 16384 -w 8 > ../../../results/write/concurrent_same/8_5

# 16 workers
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 64 -w 16 > ../../../results/write/concurrent_same/16_1
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 256 -w 16 > ../../../results/write/concurrent_same/16_2
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 1024 -w 16 > ../../../results/write/concurrent_same/16_3
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 4096 -w 16 > ../../../results/write/concurrent_same/16_4
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 16384 -w 16 > ../../../results/write/concurrent_same/16_5

# 32 workers
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 64 -w 32 > ../../../results/write/concurrent_same/32_1
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 256 -w 32 > ../../../results/write/concurrent_same/32_2
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 1024 -w 32 > ../../../results/write/concurrent_same/32_3
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 4096 -w 32 > ../../../results/write/concurrent_same/32_4
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 1 -s 16384 -w 32 > ../../../results/write/concurrent_same/32_5



# Test: concurrent write - diff key
echo "${BLUE} Starting Concurrent Write - Different Key Tests${NOCOLOR}"
# 2 workers
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 64 -w 2 > ../../../results/write/concurrent_diff/2_1
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 256 -w 2 > ../../../results/write/concurrent_diff/2_1
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 1024 -w 2 > ../../../results/write/concurrent_diff/2_3
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 4098 -w 2 > ../../../results/write/concurrent_diff/2_4
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 16384 -w 2 > ../../../results/write/concurrent_diff/2_5

# 4 workers
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 64 -w 4 > ../../../results/write/concurrent_diff/4_1
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 256 -w 4 > ../../../results/write/concurrent_diff/4_2
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 1024 -w 4 > ../../../results/write/concurrent_diff/4_3
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 4096 -w 4 > ../../../results/write/concurrent_diff/4_4
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 16384 -w 4 > ../../../results/write/concurrent_diff/4_5

# 8 workers
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 64 -w 8 > ../../../results/write/concurrent_diff/8_1
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 256 -w 8 > ../../../results/write/concurrent_diff/8_2
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 1024 -w 8 > ../../../results/write/concurrent_diff/8_3
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 4096 -w 8 > ../../../results/write/concurrent_diff/8_4
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 16384 -w 8 > ../../../results/write/concurrent_diff/8_5

# 16 workers
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 64 -w 16 > ../../../results/write/concurrent_diff/16_1
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 256 -w 16 > ../../../results/write/concurrent_diff/16_2
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 1024 -w 16 > ../../../results/write/concurrent_diff/16_3
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 4096 -w 16 > ../../../results/write/concurrent_diff/16_4
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 16384 -w 16 > ../../../results/write/concurrent_diff/16_5

# 32 workers
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 64 -w 32 > ../../../results/write/concurrent_diff/32_1
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 256 -w 32 > ../../../results/write/concurrent_diff/32_2
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 1024 -w 32 > ../../../results/write/concurrent_diff/32_3
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 4096 -w 32 > ../../../results/write/concurrent_diff/32_4
./write 0.0.0.0:50051 -k aaaaaaaaaaaaaaaaaaa -i 100 -t 2 -s 16384 -w 32 > ../../../results/write/concurrent_diff/32_5
