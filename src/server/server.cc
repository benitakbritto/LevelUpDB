#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cassert>
#include <memory>
#include <cerrno>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <mutex>
#include <filesystem>
#include <fstream>
#include <typeinfo>
#include <shared_mutex>
#include <chrono>
#include <grpc++/grpc++.h>
#include <grpcpp/health_check_service_interface.h>

#include "raft.grpc.pb.h"
#include "lb.grpc.pb.h"
#include "../util/common.h"

/******************************************************************************
 * NAMESPACES
 *****************************************************************************/
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::ServerReader;
using grpc::Status;
using grpc::ClientContext;
using grpc::ClientReaderWriter;
using namespace blockstorage;
using namespace std;

/******************************************************************************
 * MACROS
 *****************************************************************************/

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
string self_addr_lb = "0.0.0.0:50051";
string lb_addr = "0.0.0.0:50056";

class LBNodeCommClient {
  private:
    unique_ptr<LBNodeComm::Stub> stub_;
    Identity identity;
    string ip;
  
  public:
    LBNodeCommClient(string target_str, Identity _identity, string _ip) {
      identity = _identity;
      stub_ = LBNodeComm::NewStub(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
      ip = _ip;
    }

    void SendHeartBeat() {
        ClientContext context;
        
        std::shared_ptr<ClientReaderWriter<HeartBeatRequest, HeartBeatReply> > stream(
            stub_->SendHeartBeat(&context));

        HeartBeatReply reply;
        HeartBeatRequest request;
        request.set_ip(ip);

        while(1) {
          request.set_identity(identity);
          stream->Write(request);
          dbgprintf("INFO] SendHeartBeat: sent heartbeat\n");

          stream->Read(&reply);
          dbgprintf("[INFO] SendHeartBeat: recv heartbeat response\n");
          
          // TODO : Parse reply to get sys state

          dbgprintf("[INFO] SendHeartBeat: sleeping for 5 sec\n");
          sleep(HB_SLEEP_IN_SEC);
        }
    }
};

void *StartHB(void* args) {
    Identity identity_enum = LEADER;

    LBNodeCommClient lBNodeCommClient(lb_addr, identity_enum, self_addr_lb);
    lBNodeCommClient.SendHeartBeat();

    return NULL;
}


// void RunServer() {
//     std::string server_address("0.0.0.0:50051");
//     ServerImplementation service;

//     ServerBuilder builder;
//     builder.SetMaxReceiveMessageSize((1.5 * 1024 * 1024 * 1024));
//     builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
//     builder.RegisterService(&service);

//     std::unique_ptr<Server> server(builder.BuildAndStart());
//     std::cout << "Server listening on " << server_address << std::endl;
//     server->Wait();
// }

int main(int argc, char **argv) {
    pthread_t hb_t;
    pthread_create(&hb_t, NULL, StartHB, NULL);
    pthread_join(hb_t, NULL);
    return 0;
}