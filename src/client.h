#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <memory>
#include <string>
#include "util/common.h"
#include <grpcpp/grpcpp.h>
#ifdef BAZEL_BUILD
#include "examples/protos/keyvalueops.grpc.pb.h"
#else
#include "keyvalueops.grpc.pb.h"
#endif

/******************************************************************************
 * NAMESPACES
 *****************************************************************************/
using namespace std;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using blockstorage::PutRequest;
using blockstorage::PutReply;
using blockstorage::GetRequest;
using blockstorage::GetReply;
using blockstorage::KeyValueOps;

using namespace std;

class KeyValueClient  
{

private:
    unique_ptr<KeyValueOps::Stub> stub_;
    
public:
    KeyValueClient(std::shared_ptr<Channel> channel)
      : stub_(KeyValueOps::NewStub(channel)) {}
    
    // void Connect(string target);
    Status GetFromDB(GetRequest request, GetReply *reply);
    Status PutToDB(PutRequest request, PutReply *reply);
};

#endif