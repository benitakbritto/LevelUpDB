#include "client.h"
#include <iostream>
#include <stdlib.h>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "../util/common.h"
#ifdef BAZEL_BUILD
#include "examples/protos/keyvalueops.grpc.pb.h"
#else
#include "keyvalueops.grpc.pb.h"
#endif

/******************************************************************************
 * NAMESPACES
 *****************************************************************************/
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using namespace kvstore;

/******************************************************************************
 * DRIVER
 *****************************************************************************/
// @usage for put:                              ./keyvalue_client <ip of lb with port> -p <key> <val>
// @usage for get with strong consistency:      ./keyvalue_client <ip of lb with port> -g <key> 0
// @usage for get with eventual consistency:    ./keyvalue_client <ip of lb with port> -g <key> 1 <quorum>
int main(int argc, char** argv) 
{
    string target_str = string(argv[1]);
    KeyValueClient* keyValueClient = new KeyValueClient(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    
    string operation = argv[2];

    if(operation.compare("-p")==0)
    {
        // Test Put
        PutRequest putRequest;
        PutReply putReply;

        putRequest.set_key(argv[3]);
        putRequest.set_value(argv[4]);

        Status putStatus = keyValueClient->PutToDB(putRequest, &putReply);
        cout << "Error code: " << putStatus.error_code() << endl;
    }

    else {
        // Test Get
        GetRequest getRequest;
        GetReply getReply;
        string key;
        int consistencyLevel;
        int quorum;

        key = argv[3];
        consistencyLevel = stoi(argv[4]); 

        if(consistencyLevel==1) 
        {   // eventual consistency
            quorum = stoi(argv[5]);

        }
        
        getRequest.set_key(key);
        getRequest.set_consistency_level(consistencyLevel);
        getRequest.set_quorum(quorum);

        Status getStatus = keyValueClient->GetFromDB(getRequest, &getReply);
        cout << "Error code: " << getStatus.error_code() << endl;
        
        for(int i=0; i < getReply.values().size(); i++)
        {
            cout << getReply.values(i).value() << endl;
        }

    }
    return 0;
}