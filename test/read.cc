// TODO: Test code thoroughly

/******************************************************************************
 * @usage: ./read <kv lb ip> 
 *                -k <key> 
 *                -i <iter> 
 *                -t <testCase> 
 *                -l <consistency level> 
 *                -q <quorum size (only for eventual consistency)>
 *                -w <number of workers
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
    SINGLE_READ,
    CONCURRENT_READ_SAME,
    CONCURRENT_READ_DIFF
};

KeyValueClient* keyValueClient;

/******************************************************************************
 * MACROS
 *****************************************************************************/
#define DEFAULT_VALUE_SIZE 100


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
void testSingleRead(string key, int consistencyLevel, int quorumSize, int iterations);
void testConcurrentReadOnSameKey(string key, int consistencyLevel, int quorumSize, int iterations, int numOfWorkers);
void testConcurrentReadOnDifferentKey(string startKey, int consistencyLevel, int quorumSize, int iterations, int numOfWorkers);
void putKeyIfNotExists(string key);
string generateValue();

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
    int consistencyLevel = -1;
    char c = '\0';
    int numOfWorkers = 0;
    int quorumSize = 0;

    // Get command line args
    while ((c = getopt(argc, argv, "k:i:t:l:w:q:")) != -1)
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
            case 'l':
                consistencyLevel = stoi(optarg);
                break;
            case 'w':
                numOfWorkers = stoi(optarg);
                break;
            case 'q':
                quorumSize = stoi(optarg);
                break;
            default:
                cout << "Invalid arg" << endl;
                return -1;
        }
    }


    // Run test
    switch(testCase)
    {
        case SINGLE_READ:
            cout << "Testing Single Read" << endl;
            testSingleRead(key, consistencyLevel, quorumSize, iterations);
            break;
        case CONCURRENT_READ_SAME:
            cout << "Testing Concurrent Read on same key" << endl;
            testConcurrentReadOnSameKey(key, consistencyLevel, quorumSize, iterations, numOfWorkers);
            break;
        case CONCURRENT_READ_DIFF:
            cout << "Testing Concurrent Read on different keys" << endl;
            testConcurrentReadOnDifferentKey(key, consistencyLevel, quorumSize, iterations, numOfWorkers);
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

void testSingleRead(string key, int consistencyLevel, int quorumSize, int iterations)
{
    putKeyIfNotExists(key);

    GetRequest getRequest;
    GetReply getReply;
    Status getStatus;

    getRequest.set_key(key);
    getRequest.set_consistency_level(consistencyLevel);
    getRequest.set_quorum(quorumSize);

    for (int i = 0; i < iterations; i++)
    {
        auto start = steady_clock::now();
        getStatus = keyValueClient->GetFromDB(getRequest, &getReply);
        auto end = steady_clock::now();

        if (getStatus.error_code() != 0)
        {
            cout << "[ERROR] " << __func__ << " failed. Stopping test." << endl;
            return;
        }
        else
        {
            nanoseconds elapsedTime = end - start;
            printTime(elapsedTime);
        }
    }
}

void testConcurrentReadOnSameKey(string key, int consistencyLevel, int quorumSize, int iterations, int numOfWorkers)
{
    future<void> workers[numOfWorkers];
    putKeyIfNotExists(key);

    for (int i = 0; i < numOfWorkers; i++) 
    {
        workers[i] = async(testSingleRead, key, consistencyLevel, quorumSize, iterations); 
    }

    for(int i = 0; i < numOfWorkers; i++) 
    {
        workers[i].get();
    }
}

void testConcurrentReadOnDifferentKey(string startKey, int consistencyLevel, int quorumSize, int iterations, int numOfWorkers)
{
    string key = "";
    future<void> workers[numOfWorkers];

    for (int i = 0; i < numOfWorkers; i++) 
    {
        key = startKey + to_string(i);
        putKeyIfNotExists(key);
        workers[i] = async(testSingleRead, key, consistencyLevel, quorumSize, iterations); 
    }

    for(int i = 0; i < numOfWorkers; i++) 
    {
        workers[i].get();
    }
}

// For safety
void putKeyIfNotExists(string key)
{
    GetRequest getRequest;
    GetReply getReply;
    Status getStatus;

    getRequest.set_key(key);
    getRequest.set_consistency_level(0);
    getRequest.set_quorum(0);

    getStatus = keyValueClient->GetFromDB(getRequest, &getReply);
    // TODO: Check what is being returned if key does not exist
    if (getStatus.error_code() != 0)
    {
        PutRequest putRequest;
        PutReply putReply;

        putRequest.set_key(key);
        putRequest.set_value(generateValue());

        Status putStatus = keyValueClient->PutToDB(putRequest, &putReply);
        if (putStatus.error_code() != 0)
        {
            cout << "[ERROR] Put failed. Exiting program." << endl;
            exit(-1);
        }
    }
}

string generateValue()
{
    string value = "";
    for (int i = 0; i < DEFAULT_VALUE_SIZE; i++)
    {
        value += "a";
    }

    return value;
}