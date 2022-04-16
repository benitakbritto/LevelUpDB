#ifndef COMMON_H
#define COMMON_H

/******************************************************************************
 * MACROS
 *****************************************************************************/
#define DEBUG                       0                  
#define dbgprintf(...)              if (DEBUG) { printf(__VA_ARGS__); }
#define CRASH_TEST                  0
#define crash()                     if (CRASH_TEST) { *((char*)0) = 0; }

#endif
