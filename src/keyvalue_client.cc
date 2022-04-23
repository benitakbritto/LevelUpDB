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

int main(int argc, char** argv) 
{
    string target_str = "0.0.0.0:50051"; // LoadBalancer - acting as server for client
    KeyValueClient* keyValueClient = new KeyValueClient(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    PutRequest request;
    PutReply reply;

    request.set_key("k1");
    request.set_value("v1");


    dbgprintf("[DEBUG] Client contacting LB\n");
    Status putStatus = keyValueClient->PutToDB(request, &reply);

    cout << putStatus.error_code() << endl;
     
    return 0;
}