#ifndef COMMON_H
#define COMMON_H

#include <iostream>

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
enum ServerIdentity { LEADER, FOLLOWER, CANDIDATE};
enum ConsistencyLevel { STRONG, EVENTUAL };
/******************************************************************************
 * MACROS
 *****************************************************************************/
#define DEBUG                       1                
#define dbgprintf(...)              if (DEBUG) { printf(__VA_ARGS__); }
#define CRASH_TEST                  0
#define crash()                     if (CRASH_TEST) { *((char*)0) = 0; }
// #define REPLICATED_LOG_PATH         "/home/benitakbritto/CS739-P4/storage/replicated_log"
// #define TERM_VOTE_PATH              "/home/benitakbritto/CS739-P4/storage/term_vote"
#define HB_SLEEP_IN_SEC             3   
#define REPLICATED_LOG_PATH         "/users/ssharma/CS739-P4/storage/replicated_log"
#define TERM_VOTE_PATH              "/users/ssharma/CS739-P4/storage/term_vote"
#define LEADER_STR                  "LEADER"
#define FOLLOWER_STR                "FOLLOWER"
#define RETRY_TIME_START             1                                     
#define RETRY_TIME_MULTIPLIER        2                                    

/******************************************************************************
 * NAMESPACES
 *****************************************************************************/
using namespace std;

#endif
