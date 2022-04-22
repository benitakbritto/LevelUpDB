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

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
unordered_map<string, pair<int, KeyValueClient*>> nodes; // <ip: <id, stub>>
string leaderIP;

/******************************************************************************
 * DECLARATION
 *****************************************************************************/
class KeyValueService final : public KeyValueOps::Service {
    private:
        // for round-robin
        int idx = 0;

        KeyValueClient* getServerIPToRouteTo(){
            idx = (++idx) % (nodes.size());
            dbgprintf("[INFO]: current idx is: %d\n", idx);
            auto it = nodes.begin();
            advance(it, idx);
            return it->second.second;
        }

        Status GetFromDB(ServerContext* context, const GetRequest* request, GetReply* reply) override {
            KeyValueClient* stub = getServerIPToRouteTo();
            return stub->GetFromDB(*request, reply);
        }

        Status PutToDB(ServerContext* context,const PutRequest* request, PutReply* reply) override {
            dbgprintf("Reached LB Put \n");
            KeyValueClient* stub = nodes[leaderIP].second;
            dbgprintf("%d %s \n", stub == NULL, leaderIP.c_str());
            return stub->PutToDB(*request, reply);
        } 

    public:
        KeyValueService(){}

};

/******************************************************************************
 * DECLARATION
 *****************************************************************************/
class LBNodeCommService final: public LBNodeComm::Service {
    
    private:

        void registerNode(int identity, string ip) {
            nodes[ip] = make_pair(identity,
                            new KeyValueClient (grpc::CreateChannel(ip, grpc::InsecureChannelCredentials())));
        }

        void eraseNode(string ip) {
            nodes.erase(ip);
        }

        void updateLeader(string ip) {
            if(!leaderIP.empty()) {
                nodes[leaderIP].first = FOLLOWER;
            }
            leaderIP = ip;
            nodes[leaderIP].first = LEADER;
        }

        void addNodeDataToReply(HeartBeatReply* reply) {
            NodeData* nodeData;
            for (auto& it: nodes) 
            {
                nodeData = reply->add_node_data();
                nodeData->set_ip(it.first);
                nodeData->set_identity(it.second.first);
            }
        }

    public:
        LBNodeCommService() {}

        Status SendHeartBeat(ServerContext* context, ServerReaderWriter<HeartBeatReply, HeartBeatRequest>* stream) override {
            HeartBeatRequest request;
            HeartBeatReply reply;

            int identity;
            string ip;
            bool registerFirstTime = true;

            while(1) {
                if(!stream->Read(&request)) {
                    break;
                }
                dbgprintf("[INFO]: recv heartbeat from IP:[%s]\n", request.ip().c_str());

                identity = request.identity();
                ip = request.ip();

                if(registerFirstTime) {
                    registerNode(identity, ip);
                }
                registerFirstTime = false;
                
                if(identity == LEADER){
                    updateLeader(ip);
                }

                addNodeDataToReply(&reply);
                
                if(!stream->Write(reply)) {
                    break;
                }
                dbgprintf("[INFO]: sent heartbeat reply\n");
            }

            cout << "[ERROR]: stream broke" << endl;
            eraseNode(ip);

            return Status::OK;
        }
};

void* RunServerForClient(void* arg) {
    string server_address("0.0.0.0:50051");
    KeyValueService service;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    unique_ptr<Server> server(builder.BuildAndStart());
    cout << "Server for Client listening on " << server_address << std::endl;

    server->Wait();

    return NULL;
}

void* RunServerForNodes(void* arg) {
    string server_address("0.0.0.0:50052");
    LBNodeCommService service;
    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    unique_ptr<Server> server(builder.BuildAndStart());
    cout << "Server for Nodes listening on " << server_address << std::endl;

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