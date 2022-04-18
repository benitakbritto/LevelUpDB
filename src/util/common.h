#ifndef COMMON_H
#define COMMON_H

#include <iostream>

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
// enum Identity { LEADER, FOLLOWER, CANDIDATE};

/******************************************************************************
 * MACROS
 *****************************************************************************/
#define DEBUG                       1                 
#define dbgprintf(...)              if (DEBUG) { printf(__VA_ARGS__); }
#define CRASH_TEST                  0
#define crash()                     if (CRASH_TEST) { *((char*)0) = 0; }
#define REPLICATED_LOG_PATH         "/home/benitakbritto/CS739-P4/storage/replicated_log"
#define TERM_VOTE_PATH              "/home/benitakbritto/CS739-P4/storage/term_vote"
#define HB_SLEEP_IN_SEC             5
#define LEADER_STR                  "LEADER"
#define FOLLOWER_STR                "FOLLOWER"

/******************************************************************************
 * NAMESPACES
 *****************************************************************************/
using namespace std;

#endif