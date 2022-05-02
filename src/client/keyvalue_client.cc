#include "client.h"
#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "../util/common.h"
#include "keyvalueops.grpc.pb.h"
#include "resalloc.grpc.pb.h"


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
// @usage: ./keyvalue_client <ip of kv lb with port> <ip of resalloc lb with port>
int main(int argc, char** argv) 
{
    // string target_str = string(argv[1]);
    // KeyValueClient* keyValueClient = new KeyValueClient(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    
    // Test Put
    // PutRequest putRequest;
    // PutReply putReply;

    // putRequest.set_key("k1");
    // putRequest.set_value("v1");

    // Status putStatus = keyValueClient->PutToDB(putRequest, &putReply);
    // cout << putStatus.error_code() << endl;

    // Test Get
    // GetRequest getRequest;
    // GetReply getReply;

    // getRequest.set_key("k1");
    // getRequest.set_consistency_level(0);

    // Status getStatus = keyValueClient->GetFromDB(getRequest, &getReply);
    // cout << getStatus.error_code() << endl;

    // Resalloc stub
    auto resAllocStub = ResAlloc::NewStub(grpc::CreateChannel(string(argv[2]), grpc::InsecureChannelCredentials()));

    // Test AddServer
    ClientContext addServercontext;
    AddServerRequest addServerRequest;
    AddServerReply addServerReply;
    Status addServerStatus = resAllocStub->AddServer(&addServercontext, addServerRequest, &addServerReply);
    cout << "status = " << addServerStatus.error_code() << endl;
    
    // Test DeleteServer
    // ClientContext deleteServerContext;
    // DeleteServerRequest deleteServerRequest;
    // DeleteServerReply deleteServerReply;
    // Status deleteServerStatus = resAllocStub->DeleteServer(&deleteServerContext, deleteServerRequest, &deleteServerReply);
    // cout << "status = " << deleteServerStatus.error_code() << endl;

    return 0;
}
