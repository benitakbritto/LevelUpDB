#ifndef TERM_VOTE_PERSISTENT_H
#define TERM_VOTE_PERSISTENT_H

#include "common.h"
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <vector>

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
struct TermVoteEntry
{
    int term;
    string votedFor;
    TermVoteEntry(int term, string votedFor) : term(term), votedFor(votedFor) {}
};

typedef struct TermVoteEntry TVEntry;

/******************************************************************************
 * MACROS
 *****************************************************************************/
#define DELIM                   ","

/******************************************************************************
 * NAMESPACES
 *****************************************************************************/



/******************************************************************************
 * DECLARATION
 *****************************************************************************/
class PersistentTermVote
{
private:
    int fd;
    void WriteToLog(int term, string ip);
    string CreateLogEntry(int term, string ip);
    
public:
    PersistentTermVote();
    void AddTerm(int term);
    void AddVotedFor(int term, string ip);
    vector<TVEntry> ParseLog();
};

#endif