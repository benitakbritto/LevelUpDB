// TODO: Reader-writer locks
// TODO: Add to cmake

#include "replicated_log_persistent.h"

/*
*   @brief Gets file descriptor of log file
*/
PersistentReplicatedLog::PersistentReplicatedLog()
{
    string log_file_path = REPLICATED_LOG_PATH;
    fd = open(log_file_path.c_str(), O_WRONLY | O_CREAT, 0666);

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
    dbgprintf("[DEBUG]: GoToEndOfFile - Entering function\n");

    if (lseek(fd, 0, SEEK_END) == -1)
    {
        throw runtime_error("[ERROR]: lseek() failed");
    }

    dbgprintf("[DEBUG]: GoToEndOfFile - Exiting function\n");
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
*   @brief Write entry to log
*
*   @param term 
*   @param key 
*   @param value 
*   @param offset 
*/
void PersistentReplicatedLog::WriteToLog(int term, string key, string value, int offset)
{
    dbgprintf("[DEBUG]: WriteToLog - Entering function\n");

    string log_entry = "";
    int res = 0;

    log_entry = CreateLogEntry(term, key, value);
    res = pwrite(fd, log_entry.c_str(), log_entry.size(), offset);
    lseek(fd, res, SEEK_CUR); // move bytes written ahead
    
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

/*
*   @brief Add command entry to the end of the log
*
*   @param term 
*   @param key 
*   @param value 
*   @param offset
*/
void PersistentReplicatedLog::Append(int term, string key, string value, int offset)
{
    dbgprintf("[DEBUG]: Append - Entering function\n");

    GoToEndOfFile();
    WriteToLog(term, key, value, offset);

    dbgprintf("[DEBUG]: Append - Exiting function\n");
}

/*
*   @brief Add command entry to log at specified offset
*
*   @param term 
*   @param key 
*   @param value 
*/
void PersistentReplicatedLog::Insert(int offset, int term, string key, string value)
{
    dbgprintf("[DEBUG]: Insert - Entering function\n");

    GoToOffset(offset);
    WriteToLog(term, key, value, offset);

    dbgprintf("[DEBUG]: Insert - Exiting function\n");
}

/*
*   @brief Move file position to specified offset
*
*   @param offset 
*/
void PersistentReplicatedLog::GoToOffset(int offset)
{
    dbgprintf("[DEBUG]: GoToOffset - Entering function\n");

    if (lseek(fd, offset, SEEK_SET) == -1)
    {
        throw runtime_error("[ERROR]: lseek failed\n");
    }

    dbgprintf("[DEBUG]: GoToOffset - Exiting function\n");
}

/*
*   @brief Get the offset of the last byte
*
*   @return offset
*/
int PersistentReplicatedLog::GetEndOfFileOffset()
{
    return lseek(fd, 0, SEEK_END);
}

/*
*   @brief Get the current position of the file
*
*   @return offset 
*/
int PersistentReplicatedLog::GetCurrentFileOffset()
{
    return lseek(fd, 0, SEEK_CUR);
}