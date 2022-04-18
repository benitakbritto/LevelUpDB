// TODO: Add locks
// TODO: Add to cmake
#include "term_vote_persistent.h"

PersistentTermVote::PersistentTermVote()
{
    string file_path = TERM_VOTE_PATH;
    fd = open(file_path.c_str(), O_WRONLY |  O_APPEND | O_CREAT, 0666);

    if (fd == -1) 
    {
       	cout << errno << endl;
	    throw runtime_error("[ERROR]: Could not create log file");
    }

    dbgprintf("[INFO]: Init logging at: %s\n", file_path.c_str());
}

void PersistentTermVote::AddTerm(int term)
{
    WriteToLog(term, "");
}

void PersistentTermVote::AddVotedFor(int term, string ip)
{
    WriteToLog(term, ip);
}

string PersistentTermVote::CreateLogEntry(int term, string ip)
{
    return to_string(term) + DELIM + ip + "\n";
}

void PersistentTermVote::WriteToLog(int term, string ip)
{
    dbgprintf("[DEBUG]: WriteToLog - Entering function\n");

    string log_entry = "";
    int res = 0;

    log_entry = CreateLogEntry(term, ip);
    res = write(fd, log_entry.c_str(), log_entry.size());
    
    if (res == -1) 
    {
        dbgprintf("[ERROR]: WriteToLog - failed to write %s\n", log_entry.c_str());
        dbgprintf("[ERROR]: WriteToLog - Error received - %s\n", strerror(errno));
    } 
    else
    {
        fsync(fd);
    }

    dbgprintf("[DEBUG]: WriteToLog - Exiting function\n");
}

vector<TVEntry> PersistentTermVote::ParseLog()
{
    dbgprintf("[DEBUG]: ParseLog - Entering function\n");
    vector<TVEntry> ret;
    ifstream file(TERM_VOTE_PATH);

    if (file.is_open())
    {
        string line;
        while (getline(file, line))
        {
            int term_start = 0;
            int term_end = 0;
            int voted_start = 0;
            int voted_end = 0;
            int term = 0;;
            string votedFor = "";
            
            // get term
            term_end = line.find(DELIM);
            term = atoi(line.substr(term_start, term_end).c_str());
            dbgprintf("[DEBUG] term = %d\n", term);

            // get voted for
            voted_start = term_end + 1;
            voted_end = line.find(DELIM, voted_start);
            votedFor = line.substr(voted_start, voted_end - term_end - 1);
            dbgprintf("[DEBUG] voted for = %s\n", votedFor.c_str());

            ret.push_back(TVEntry(term,votedFor));
        }
    }

    dbgprintf("[DEBUG]: ParseLog - Exiting function\n");
    return ret;
}