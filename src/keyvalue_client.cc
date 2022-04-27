#include "client.h"
#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "util/common.h"
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

using kvstore::GetRequest;
using kvstore::GetReply;
using kvstore::PutRequest;
using kvstore::PutReply;

/******************************************************************************
 * DRIVER
 *****************************************************************************/
// @usage: ./keyvalue_client <ip of lb with port>
int main(int argc, char** argv) 
{
    string target_str = string(argv[1]);
    KeyValueClient* keyValueClient = new KeyValueClient(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    
    // Test Put
    PutRequest putRequest;
    PutReply putReply;

    putRequest.set_key("k1");
    putRequest.set_value("v1");

    Status putStatus = keyValueClient->PutToDB(putRequest, &putReply);
    cout << putStatus.error_code() << endl;

    // Test Get
    // GetRequest getRequest;
    // GetReply getReply;

    // getRequest.set_key("k1");
    // getRequest.set_consistency_level(0);

    // Status getStatus = keyValueClient->GetFromDB(getRequest, &getReply);
    // cout << getStatus.error_code() << endl;

    return 0;
}
