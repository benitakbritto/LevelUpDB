// TODO: Reader-writer locks

#include "replicated_log.h"

/*
*   @brief Gets file descriptor of log file
*/
ReplicatedLog::ReplicatedLog()
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
*   @brief Create command entry string
*
*   @param term 
*   @param key 
*   @param value 
*   @return command entry string
*/
string ReplicatedLog::CreateLogEntry(int term, string key, string value)
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
void ReplicatedLog::Append(int term, string key, string value)
{
    dbgprintf("[DEBUG]: Append - Entered function\n");
    string log_entry = CreateLogEntry(term, key, value);
    int res = write(fd, log_entry.c_str(), log_entry.size());

    if (res == -1) {
        dbgprintf("[ERROR]: Append - failed to write %s\n", log_entry.c_str());
        dbgprintf("[ERROR]: Append - Error received - %s\n", strerror(errno));
    } else {
        fsync(fd);
    }

    dbgprintf("[DEBUG]: Append - Exiting function\n");
}

/*
*   @brief Tester - Uncomment to test individually
*   @usage g++ -o replicated_log replicated_log.cc -Wall
*/
// int main()
// {
//     ReplicatedLog obj;
//     obj.Append(1, "key", "value");
//     obj.Append(2, "key", "value");
//     obj.Append(3, "key", "value");
//     return 0;
// }