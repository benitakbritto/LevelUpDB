#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "resalloc.grpc.pb.h"
#include "../util/common.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using namespace kvstore;
using namespace std;

int main(int argc, char** argv) 
{
    string target_str;
    target_str = "localhost:50000"; // TODO: Use Macro
    ClientContext context;
    AddServerRequest addServerRequest;
    AddServerReply addServerReply;

    auto stub = ResAlloc::NewStub(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    Status status = stub->AddServer(&context, addServerRequest, &addServerReply);
    cout << "status = " << status.error_code() << endl;
    return 0;
}