// TODO: Test code thoroughly

/******************************************************************************
 * @usage: ./write <kv lb ip> 
 *                -k <key> 
 *                -i <iter> 
 *                -t <testCase> 
 *                -s <value size in bytes> 
 *                -w <number of workers>
 * where TODO
 *  
 *  
 * 
 *  
 *  
 * 
 *  @prereq: Things to do before running this test TODO
 * 
 *  
 *****************************************************************************/

#include <iostream>
#include <chrono>
#include "../src/client/client.h"
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include <unistd.h>
#include <functional>
#include <future>

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
enum TestCase
{
    SINGLE_WRITE,
    CONCURRENT_WRITE_SAME,
    CONCURRENT_WRITE_DIFF
};

KeyValueClient* keyValueClient;

/******************************************************************************
 * MACROS
 *****************************************************************************/


/******************************************************************************
 * NAMESPACES
 *****************************************************************************/
using namespace std;
using namespace chrono;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using namespace kvstore;

/******************************************************************************
 * PROTOTYPES
 *****************************************************************************/
void printTime(nanoseconds elapsed_time);
void testSingleWrite(string key, int valueSize, int iterations);
void testConcurrentWriteOnSameKeySameValueSize(string key, int valueSize, int iterations, int numOfWorkers);
void testConcurrentWriteOnDifferentKeySameValueSize(string startKey, int valueSize, int iterations, int numOfWorkers);
string generateValue(int size);

/******************************************************************************
 * DRIVER
 *****************************************************************************/
int main(int argc, char** argv) 
{
    // Init
    string kvLbAddr = string(argv[1]);
    keyValueClient = new KeyValueClient(grpc::CreateChannel(kvLbAddr, grpc::InsecureChannelCredentials()));
    
    string key = "";
    int iterations = 0;
    int testCase = -1;
    int valueSize = 0;
    char c = '\0';
    int numOfWorkers = 0;

    // Get command line args
    while ((c = getopt(argc, argv, "k:i:t:s:w:")) != -1)
    {
        switch (c)
        {
            case 'k':
                key = string(optarg);
                break;
            case 'i':
                iterations = stoi(optarg);
                break;
            case 't':
                testCase = stoi(optarg);
                break;
            case 's':
                valueSize = stoi(optarg);
                break;
            case 'w':
                numOfWorkers = stoi(optarg);
                break;
            default:
                cout << "Invalid arg" << endl;
                return -1;
        }
    }


    // Run test
    switch(testCase)
    {
        case SINGLE_WRITE:
            cout << "Testing Single Write" << endl;
            testSingleWrite(key, valueSize, iterations);
            break;
        case CONCURRENT_WRITE_SAME:
            cout << "Testing Concurrent Write on same key" << endl;
            testConcurrentWriteOnSameKeySameValueSize(key, valueSize, iterations, numOfWorkers);
            break;
        case CONCURRENT_WRITE_DIFF:
            cout << "Testing Concurrent Write on different keys" << endl;
            testConcurrentWriteOnDifferentKeySameValueSize(key, valueSize, iterations, numOfWorkers);
            break;
        default:
            cout << "Invalid arg" << endl;
            return -1;
    }

    return 0;
}

/******************************************************************************
 * DEFINITIONS
 *****************************************************************************/
void printTime(nanoseconds elapsed_time)
{
    cout << (elapsed_time.count() / 1e6) << endl;
}

void testSingleWrite(string key, int valueSize, int iterations)
{
    PutRequest putRequest;
    PutReply putReply;
    Status putStatus;

    putRequest.set_key(key);
    putRequest.set_value(generateValue(valueSize));

    for (int i = 0; i < iterations; i++)
    {
        auto start = steady_clock::now();
        putStatus = keyValueClient->PutToDB(putRequest, &putReply);
        auto end = steady_clock::now();

        nanoseconds elapsedTime = end - start;
        printTime(elapsedTime);

        if (putStatus.error_code() != 0)
        {
            cout << "[ERROR] " << __func__ << " failed. Stopping test." << endl;
            return;
        }
    }
}

void testConcurrentWriteOnSameKeySameValueSize(string key, int valueSize, int iterations, int numOfWorkers)
{
    future<void> workers[numOfWorkers];

    for (int i = 0; i < numOfWorkers; i++) 
    {
        workers[i] = async(testSingleWrite, key, valueSize, iterations); 
    }

    for(int i = 0; i < numOfWorkers; i++) 
    {
        workers[i].get();
    }
}

void testConcurrentWriteOnDifferentKeySameValueSize(string startKey, int valueSize, int iterations, int numOfWorkers)
{
    string key = "";
    future<void> workers[numOfWorkers];

    for (int i = 0; i < numOfWorkers; i++) 
    {
        key = startKey + to_string(i);
        workers[i] = async(testSingleWrite, key, valueSize, iterations); 
    }

    for(int i = 0; i < numOfWorkers; i++) 
    {
        workers[i].get();
    }
}

string generateValue(int size)
{
    string value = "";
    for (int i = 0; i < size; i++)
    {
        value += "a";
    }

    return value;
}