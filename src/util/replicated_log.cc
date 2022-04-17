#include "replicated_log.h"

void ReplicatedLogHelper::Append(int term, string key, string value)
{
    int offset = 0;

    offset = pObj.GetEndOfFileOffset();
    pObj.Append(term, key, value);
    vObj.Append(term, key, value, offset);
}

/*
*   @brief Tester - Uncomment to test individually
*   @usage g++ replicated_log.cc replicated_log_persistent.cc replicated_log_volatile.cc -Wall -o log && ./log
*/
int main()
{
    ReplicatedLogHelper obj;
    obj.Append(1, "key1", "value");
    obj.Append(2, "key2", "value");
    obj.Append(3, "key3", "value");
    return 0;
}