#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "keyvalueops.grpc.pb.h"
#include "lb.grpc.pb.h"
#include "client.h"
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>
#include "util/common.h"

/******************************************************************************
 * NAMESPACES
 *****************************************************************************/
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerReaderWriter;
using namespace blockstorage;
using namespace std;

/******************************************************************************
 * MACROS
 *****************************************************************************/
#define LEADER_STR "LEADER"
#define FOLLOWER_STR "FOLLOWER"

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
map<string, string> live_servers;
map<string, KeyValueClient*> kv_clients;

/******************************************************************************
 * DECLARATION
 *****************************************************************************/
class KeyValueService final : public KeyValueOps::Service {
    private:
        // for round-robin
        int idx = 0;
        map<string, string>* nodes;
        map<string, KeyValueClient*>* kv_clients;

        void print_map() {
            for (std::map<string,string>::iterator it=nodes->begin(); it!=nodes->end(); ++it)
                dbgprintf("%s => %s\n", it->first.c_str(), it->second.c_str());
        }
        
        bool is_registered(string ip) {
            return !((*nodes)[ip].empty());
        }

        string getServerToRouteTo(){
            idx = (++idx) % (nodes->size());
            dbgprintf("[INFO]: current idx is: %d\n", idx);
            auto it = nodes->begin();
            advance(it, idx);
            return it->first.c_str();
        }

    public:
        KeyValueService(map<string, string> &servers, map<string, KeyValueClient*> &clients){
            nodes = &servers;
            kv_clients = &clients;
        }

};

/******************************************************************************
 * DECLARATION
 *****************************************************************************/
class LBNodeCommService final: public LBNodeComm::Service {
    private:
        map<string, string> nodes; //ip:identity map
        string leaderIP;

        void registerNode(string identity, string target_str) {
            nodes[target_str] = identity;
        }

        void eraseNode(string ip) {
            nodes.erase(ip);
        }

        void updateLeader(string ip) {
            if(!leaderIP.empty()) {
                nodes[leaderIP] = FOLLOWER_STR;
            }
            leaderIP = ip;
            nodes[ip] = LEADER_STR;
        }

    public:
        LBNodeCommService(map<string, string> servers) {
            nodes = servers;
        }

        Status SendHeartBeat(ServerContext* context, ServerReaderWriter<HeartBeatReply, HeartBeatRequest>* stream) override {
            HeartBeatRequest request;
            HeartBeatReply reply;

            string prev_identity;
            string identity;
            string ip;
            bool first_time = true;
            // TODO: create client on the fly

            while(1) {
                if(!stream->Read(&request)) {
                    break;
                }
                dbgprintf("[INFO]: recv heartbeat from IP:[%s]\n", request.ip().c_str());

                identity = Identity_Name(request.identity());
                ip = request.ip();
                registerNode(identity, ip);
                
                if(identity.compare(LEADER_STR) == 0){
                    updateLeader(ip);
                }
                if(!stream->Write(reply)) {
                    break;
                }
                dbgprintf("[INFO]: sent heartbeat reply\n");
            }

            cout << "[ERROR]: stream broke" << endl;
            dbgprintf("[ERROR]: stream broke\n");
            eraseNode(ip);

            return Status::OK;
        }
};

void* RunServerForClient(void* arg) {
  int port = 50051;
  std::string server_address("0.0.0.0:" + std::to_string(port));
  KeyValueService service(live_servers, kv_clients);

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server for Client listening on " << server_address << std::endl;

  server->Wait();

  return NULL;
}

void* RunServerForNodes(void* arg) {
  int port = 50056;
  std::string server_address("0.0.0.0:" + std::to_string(port));
  LBNodeCommService service(live_servers);

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server for Nodes listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();

  return NULL;
}

/******************************************************************************
 * DRIVER
 *****************************************************************************/
int main (int argc, char *argv[]){
    pthread_t client_server_t, node_server_t;
  
    pthread_create(&client_server_t, NULL, RunServerForClient, NULL);
    pthread_create(&node_server_t, NULL, RunServerForNodes, NULL);

    pthread_join(client_server_t, NULL);
    pthread_join(node_server_t, NULL);
}