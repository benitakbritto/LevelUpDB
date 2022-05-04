#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <mutex>  
#include <grpcpp/grpcpp.h>
#include "keyvalueops.grpc.pb.h"
#include "lb.grpc.pb.h"
#include "client.h"
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>
#include "../util/common.h"
#include "../util/locks.h"
/******************************************************************************
 * NAMESPACES
 *****************************************************************************/
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;
using grpc::ServerReaderWriter; 
using namespace kvstore;
using namespace std;

/******************************************************************************
 * MACROS
 *****************************************************************************/

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
unordered_map<string, pair<int, KeyValueClient*>> g_nodeList; // <ip: <id, stub>>
unordered_map<int, pair<int, bool>> g_reqSuccessMap; // <id: <success count: quorum reached> > 
string leaderIP;
int g_currentTerm = 0;
mutex g_mtx;  
atomic<int> g_currentReqId = 0; // Monotonically increasing counter

/******************************************************************************
 * DECLARATION
 *****************************************************************************/
/*
*   @brief Replays client requests to the server node(s)
*/
class KeyValueService final : public KeyValueOps::Service 
{
private:
    int idx = 0; 
    /*
    *   @brief Gets server ip using round robin
    *
    *   @return Server stub
    */
    KeyValueClient* getServerIPToRouteTo()
    {
        idx = (++idx) % (g_nodeList.size());
        auto it = g_nodeList.begin();
        advance(it, idx);
        dbgprintf("[DEBUG] %s: Routing to %s\n", __func__, (it->first).c_str());
        return it->second.second;
    }

    /* @brief get the stub for the leader ip if leader exists, else returns NULL
    * 
    * @return stub
    */
    KeyValueClient* getLeaderStub()
    {
        if (g_nodeList.count(leaderIP) == 0)
        {
            cout << "[WARN] No leader in the system yet" << endl;
            return NULL;
        }

        return g_nodeList[leaderIP].second;
    }

    // TODO: Reads should go to leader for strong consistency
    /*
    *   @brief receive client request from client
    *                 and replay to server node(s)
    *
    *   @param context 
    *   @param request 
    *   @param response 
    *   @param offset 
    *   @return gRPC status
    */
    Status GetFromDB(ServerContext* context, const GetRequest* request, GetReply* reply) override 
    {
        dbgprintf("[DEBUG] %s: Entering function\n", __func__);
        KeyValueClient* stub;
        int consistencyLevel;
        
        consistencyLevel = request->consistency_level();
        dbgprintf("[DEBUG}: Consistency level:%d\n", consistencyLevel);
        if (consistencyLevel == STRONG)
        {
            stub = getLeaderStub();
            dbgprintf("[DEBUG}: Calling stub for leader\n");
            if (stub == NULL)
            {
                return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Leader not present");
            }
            else
            {
                return stub->GetFromDB(*request, reply);
            }
        }

        else 
        {
            int quorum = request->quorum();
            dbgprintf("Req quorum of %d\n", quorum);
         
            int reqId = ++g_currentReqId;
            g_reqSuccessMap[reqId] = make_pair(0, false);
            set<string> readVals;

            // iterate over g_nodeList
            for(auto& node: g_nodeList) 
            {
                std::thread(&KeyValueService::invokeGetFromDB, this, request->key(),
                        node.second.second, reqId, &readVals, quorum).detach();
            }

            do{
                // wait for majority
            }
            while(!receivedQuorum(reqId, quorum));

            dbgprintf("Recvd quorum\n");
            g_reqSuccessMap[reqId] = make_pair(quorum, true);

            ValueString* values;
            for(string v: readVals)
            {   
                values = reply->add_values();
                values->set_value(v);
            }
            return Status::OK;
        }
        return Status::OK;
    }

    /*
    *   @brief calculates the number of responses needed for majority
    *          
    *   @return majority count
    */
    // int GetMajorityCount() // TODO: Move to util
    // {
    //     int size = g_nodeList.size();
    //     return (size % 2 == 0) ? (size / 2) : (size / 2) + 1;
    // }

    /*
    *   @brief checks if majority responses have been received
    *          
    *   @return 
    *       true : on receiving majority or if there is only one node in the cluster
    *       false: otherwise
    */
    bool receivedQuorum(int reqId, int quorum) // TODO: Move to util
    {
        return (g_reqSuccessMap[reqId].first >= quorum);    
    }

    /*
    *   @brief sends GetFromDB to specified stub,
    *           and updates valCountMap on receiving reply
    * 
    *   @param stub
    *   @param request
    *   @param reply
    */
    void invokeGetFromDB(string key, KeyValueClient* stub, int reqId, set<string>* readVals, 
                    int quorum)
    {
        GetRequest request;
        GetReply reply;
        request.set_key(key);
        Status status = stub->GetFromDB(request, &reply);

        if (status.ok())
        {
            g_mtx.lock();
            int successCount = g_reqSuccessMap[reqId].first;
            if (successCount < quorum) {
                g_reqSuccessMap[reqId] = make_pair(successCount + 1, false);
                string v = reply.values(0).value();
                readVals->insert(v);
            }
            g_mtx.unlock();            
        }
    }

    /*
    *   @brief returns the value received from majority of g_nodeList
    * 
    *   @return value
    */
    // string getMajorityResp() {
    //     int max = 0;
    //     string value;

    //     for(auto valCount: _valCountMap)
    //     {
    //         if(valCount.second > max) 
    //         {
    //             max = valCount.second;
    //             value = valCount.first;
    //         }
    //     }

    //     return value;
    // }
    
    /*
    *   @brief receive client request from client
    *                 and replay to server node(s)
    *
    *   @param context 
    *   @param request 
    *   @param response 
    *   @param offset 
    *   @return gRPC status
    */
    Status PutToDB(ServerContext* context,const PutRequest* request, PutReply* reply) override 
    {
        dbgprintf("[DEBUG] %s: Entering function\n", __func__);
        dbgprintf("LeaderIP = %s\n", leaderIP.c_str());
        
        KeyValueClient* stub = getLeaderStub();

        if (stub == NULL)
        {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Leader not present");
        }
        return stub->PutToDB(*request, reply);
    } 

public:
    KeyValueService(){}

};

/******************************************************************************
 * DECLARATION
 *****************************************************************************/
/*
*   @brief Communication between the LB and the server g_nodeList (via heartbeats)
*/
class LBNodeCommService final: public LBNodeComm::Service 
{
private:

    /*
    *   @brief Save server node information
    *
    *   @param identity  
    *   @param ip 
    */
    void registerNode(int identity, string kvIp, string raftIp) 
    {
        g_nodeList[raftIp] = make_pair(identity,
                    new KeyValueClient (grpc::CreateChannel(kvIp, grpc::InsecureChannelCredentials())));
        
        dbgprintf("[DEBUG]: Length of g_nodeList at LB: %ld", g_nodeList.size());
    }

    /*
    *   @brief Remove node information from in-mem
    *
    *   @param ip 
    */
    void eraseNode(string raftIp) 
    {
        g_nodeList.erase(raftIp);
        dbgprintf("[DEBUG] %s: Removed ip %s, Length of g_nodeList at LB: %ld\n", __func__, raftIp.c_str(), g_nodeList.size());
    }

    /*
    *   @brief Change identity of ip
    *
    *   @param ip 
    */
    void setIdentityOfNode(int identity, string kvIp, string raftIp) 
    {
        if (g_nodeList.count(raftIp) == 0)
        {
            g_nodeList[raftIp] = make_pair(identity,
                    new KeyValueClient (grpc::CreateChannel(kvIp, grpc::InsecureChannelCredentials())));
        }
        else
        {
            g_nodeList[raftIp].first = identity;
        }
    }

    /*
    *   @brief Helper function to set reply for hearbeats
    *
    *   @param reply 
    */
    void setHeartbeatReply(HeartBeatReply* reply) 
    {
        NodeData* nodeData;
        for (auto& it: g_nodeList) 
        {
            nodeData = reply->add_node_data();
            nodeData->set_raft_ip(it.first);
            nodeData->set_identity(it.second.first);
        }
    }

    /*
    *   @brief Helper function to set reply for assert leadership
    *
    *   @param reply 
    */
    void setAssertLeadershipReply(AssertLeadershipReply* reply) 
    {
        FollowerMetadata* nodeData;
        for (auto& it: g_nodeList) 
        {
            nodeData = reply->add_follower_meta();
            nodeData->set_raft_ip(it.first);
        }
    }

public:
    LBNodeCommService() {}

    /*
    *   @brief Receives heartbeat from a server node, 
    *          registers node if LB hasn't seen the node before
    *          and sends back a list of all the alive server g_nodeList in the reply
    *
    *   @param context
    *   @param stream 
    *   @return gRPC status
    */
    Status SendHeartBeat(ServerContext* context, ServerReaderWriter<HeartBeatReply, HeartBeatRequest>* stream) override 
    {
        HeartBeatRequest request;
        HeartBeatReply reply;

        int identity;
        string raftIp;
        string kvIp;
        bool registerFirstTime = true;

        while(1) 
        {
            if(!stream->Read(&request)) 
            {
                break;
            }
            dbgprintf("[INFO] %s: recv heartbeat from IP:[%s]\n", __func__, request.raft_ip().c_str());

            identity = request.identity();
            raftIp = request.raft_ip();
            kvIp = request.kv_ip();

            if(registerFirstTime) 
            {
                dbgprintf("[DEBUG] %s: Registering node %s for the 1st time\n", __func__, raftIp.c_str());
                registerNode(identity, kvIp, raftIp);
            }

            registerFirstTime = false;
                
            setIdentityOfNode(identity, kvIp, raftIp);

            reply.Clear();
            setHeartbeatReply(&reply);
                
            if(!stream->Write(reply)) 
            {
                break;
            }
            dbgprintf("[INFO] %s: sent heartbeat reply\n", __func__);
        }

        cout << "[ERROR]: stream broke" << endl;
        // Do not delete node
        // eraseNode(raftIp);

        return Status::OK;
    }

    /*
    *   @brief Changes leader and sends leader a list of follower g_nodeList
    *
    *   @param context
    *   @param request 
    *   @param reply
    *   @return gRPC status
    */
    Status AssertLeadership(ServerContext* context,const AssertLeadershipRequest* request, AssertLeadershipReply* reply) override
    {
        dbgprintf("[DEBUG] %s: Entering function\n", __func__);
        dbgprintf("[DEBUG] %s: Previous leaderIP = %s\n", __func__, leaderIP.c_str());
        
        // Only update the leader if it is from the right term
        if (g_currentTerm < request->term())
        {
            g_currentTerm = request->term();

            // set previous leader to follower
            if (g_nodeList.count(leaderIP) != 0)
            {
                g_nodeList[leaderIP].first = FOLLOWER;
            }

            // set new leader
            leaderIP = request->leader_ip();
            dbgprintf("[DEBUG] %s: New leaderIP = %s\n", __func__, leaderIP.c_str());
            if (g_nodeList.count(leaderIP) == 0)
            {
                g_nodeList[leaderIP] = make_pair(LEADER,
                        new KeyValueClient (grpc::CreateChannel(leaderIP, grpc::InsecureChannelCredentials())));
            }
            else
            {
                g_nodeList[leaderIP].first = LEADER;
            }       
            setAssertLeadershipReply(reply);
        }
        else
        {
            dbgprintf("[INFO] %s: Bad leader\n", __func__);
        }
        dbgprintf("[DEBUG] %s: Exiting function\n", __func__);
        return Status::OK;
    }

};

/*
*   @brief Starts a service for client requests 
*/
void* RunServerForClient(void* arg) 
{
    string server_address((char *) arg); 
    KeyValueService service;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    unique_ptr<Server> server(builder.BuildAndStart());
    cout << "LB Server for Client listening on " << server_address << std::endl;

    server->Wait();

    return NULL;
}

/*
*   @brief Starts a service for server node heartbeat requests 
*/
void* RunServerForNodes(void* arg) 
{
    string server_address((char *) arg);
    LBNodeCommService service;
    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    unique_ptr<Server> server(builder.BuildAndStart());
    cout << "LB Server for Nodes listening on " << server_address << std::endl;

    server->Wait();

    return NULL;
}

/******************************************************************************
 * DRIVER
 *****************************************************************************/
int main (int argc, char *argv[]){
    pthread_t client_server_t, node_server_t;
  
    pthread_create(&client_server_t, NULL, RunServerForClient, argv[1]);
    pthread_create(&node_server_t, NULL, RunServerForNodes, argv[2]);

    pthread_join(client_server_t, NULL);
    pthread_join(node_server_t, NULL);
}