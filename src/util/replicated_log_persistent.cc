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
void PersistentReplicatedLog::GoToEndOfFile()
{
    if (lseek(fd, 0, SEEK_END) == -1)
    {
        throw runtime_error("[ERROR]: lseek() failed");
    }
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

void PersistentReplicatedLog::WriteToLog(int term, string key, string value)
{
    string log_entry = "";
    log_entry = CreateLogEntry(term, key, value);
    int res = write(fd, log_entry.c_str(), log_entry.size());
    if (res == -1) 
    {
        dbgprintf("[ERROR]: WriteToLog - failed to write %s\n", log_entry.c_str());
        dbgprintf("[ERROR]: WriteToLog - Error received - %s\n", strerror(errno));
    } 
    else
    {
        fsync(fd);
    }
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

    GoToEndOfFile();
    WriteToLog(term, key, value);

    dbgprintf("[DEBUG]: Append - Exiting function\n");
}

// void PersistentReplicatedLog::Insert(int index, int term, string key, string value)
// {
//     int offset = 0;

//     offset = volatileReplicatedLogObj.GetOffset(index);
//     GoToOffset(offset);

// }



// void PersistentReplicatedLog::SetEndOfFileOffset(int offset)
// {
//     lseek(fd, 0, SEEK_END);
// }

void PersistentReplicatedLog::GoToOffset(int offset)
{
    // offset from beginning of file
    if (lseek(fd, offset, SEEK_SET) == -1)
    {
        throw runtime_error("[ERROR]: lseek failed\n");
    }
}

int PersistentReplicatedLog::GetEndOfFileOffset()
{
    return lseek(fd, 0, SEEK_END);
}
