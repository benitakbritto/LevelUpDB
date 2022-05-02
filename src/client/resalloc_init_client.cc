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
    ClientContext context;
    AddServerRequest addServerRequest;
    AddServerReply addServerReply;
    
    addServerRequest.set_kv_ip(string(argv[1]));
    addServerRequest.set_raft_ip(string(argv[2]));
    addServerRequest.set_lb_ip(string(argv[3]));

    auto stub = ResAlloc::NewStub(grpc::CreateChannel(RES_ALLOC_SERVER_IP, grpc::InsecureChannelCredentials()));
    Status status = stub->AddServer(&context, addServerRequest, &addServerReply);
    cout << "status = " << status.error_code() << endl;
    return 0;
}