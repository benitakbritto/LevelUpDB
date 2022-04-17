#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <memory>
#include <string>
#include "util/common.h"
#include <grpcpp/grpcpp.h>
#ifdef BAZEL_BUILD
#include "examples/protos/userops.grpc.pb.h"
#else
#include "userops.grpc.pb.h"
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
using blockstorage::UserOps;

using namespace std;

class P4Client  
{

private:
    unique_ptr<UserOps::Stub> stub_;
    
public:
    P4Client(std::shared_ptr<Channel> channel)
      : stub_(UserOps::NewStub(channel)) {}
    
    // void Connect(string target);
    Status GetFromDB(GetRequest request, GetReply *reply);
    Status PutToDB(PutRequest request, PutReply *reply);
};

#endif