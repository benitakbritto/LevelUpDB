// TODO: Reader-writer locks
// TODO: Add to cmake

#include "replicated_log_persistent.h"

/*
*   @brief Gets file descriptor of log file
*/
PersistentReplicatedLog::PersistentReplicatedLog()
{
    string log_file_path = REPLICATED_LOG_PATH;
    fd = open(log_file_path.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0666);

    if (fd == -1) 
    {
       	cout << errno << endl;
	    throw runtime_error("[ERROR]: Could not create log file");
    }

    dbgprintf("[INFO]: Init logging at: %s\n", log_file_path.c_str());
}

/*
*   @brief Sets file position to the end of the file
*   @return file offset
*/
int PersistentReplicatedLog::GoToEndOfFile()
{
    int offset =  lseek(fd, 0, SEEK_END);
    if (offset == -1)
    {
        throw runtime_error("[ERROR]: lseek() failed");
    }
    return offset;
}

/*
*   @brief Create command entry string
*
*   @param term 
*   @param key 
*   @param value 
*   @return command entry string
*/
string PersistentReplicatedLog::CreateLogEntry(int term, string key, string value)
{
    return to_string(term) + DELIM + key + DELIM + value +  "\n";
}

/*
*   @brief Add command entry to the end of the log
*
*   @param term 
*   @param key 
*   @param value 
*/
void PersistentReplicatedLog::Append(int term, string key, string value)
{
    dbgprintf("[DEBUG]: Append - Entered function\n");
    
    string log_entry = "";
    int offset = 0;
   
    log_entry = CreateLogEntry(term, key, value);
    offset = GoToEndOfFile();
    int res = write(fd, log_entry.c_str(), log_entry.size());

    dbgprintf("[DEBUG]: Append - log_entry = %s\n", log_entry.c_str());
    dbgprintf("[DEBUG]: Append - offset = %d\n", offset);
    dbgprintf("[DEBUG]: Append - res = %d\n", res);

    if (res == -1) 
    {
        dbgprintf("[ERROR]: Append - failed to write %s\n", log_entry.c_str());
        dbgprintf("[ERROR]: Append - Error received - %s\n", strerror(errno));
    } 
    else 
    {
        volatileReplicatedLogObj.Append(term, key, value, offset);
        fsync(fd);
    }

    dbgprintf("[DEBUG]: Append - Exiting function\n");
}


// void PersistentReplicatedLog::SetEndOfFileOffset(int offset)
// {
//     lseek(fd, 0, SEEK_END);
// }

// void PersistentReplicatedLog::GoToOffset(int offset)
// {
//     // offset from beginning of file
//     if (lseek(fd, offset, SEEK_SET) == -1)
//     {
//         throw runtime_error("[ERROR]: lseek failed\n");
//     }
// }

// int PersistentReplicatedLog::GetEndOfFileOffset()
// {
//     return lseek(fd, 0, SEEK_END);
// }


/*
*   @brief Tester - Uncomment to test individually
*   @usage g++ replicated_log_persistent.cc replicated_log_volatile.cc -Wall -o log && ./log
*/
// int main()
// {
//     PersistentReplicatedLog obj;
//     obj.Append(1, "key", "value");
//     obj.Append(2, "key", "value");
//     obj.Append(3, "key", "value");
//     return 0;
// }