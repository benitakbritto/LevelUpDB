#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <grpcpp/grpcpp.h>
#include "keyvalueops.grpc.pb.h"
#include "lb.grpc.pb.h"
#include "resalloc.grpc.pb.h"
#include "client.h"
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>
#include "../util/common.h"
#include <vector>
#include <bits/stdc++.h>

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
#define AUTO_SCALE_THRESHOLD 3

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
enum NodeStatus
{
    ALIVE,
    DEAD
};

struct NodeMetadata
{
    string kvIp;
    KeyValueClient* kvStub;
    int identity;
    int status;
};

typedef NodeMetadata NodeMeta;

unordered_map<string, NodeMeta> g_nodeList;
// unordered_map<string, pair<int, KeyValueClient*>> g_nodeList;
string g_leaderIP;
int g_currentTerm = 0;
int shouldAutoScale = false;


/******************************************************************************
 * DECLARATION
 *****************************************************************************/
/*
*   @brief Helper
*/
class Helper {
    private:
        string _lbCommIp;
        string _resAllocIp;

    public:
        void SetNodeMetadata(int _identity, string _kvIp, string _raftIp)
        {
            dbgprintf("[DEBUG] set node meta to id: %d, kvIp: %s, raftIp: %s \n", _identity, _kvIp.c_str(), _raftIp.c_str());
            NodeMeta nodeMeta;
            nodeMeta.kvIp = _kvIp;
            nodeMeta.kvStub = new KeyValueClient(grpc::CreateChannel(_kvIp, grpc::InsecureChannelCredentials()));
            nodeMeta.identity = _identity;
            nodeMeta.status = ALIVE;
            
            g_nodeList[_raftIp] = nodeMeta;
        }

        string GetServerIPToRouteTo(int index)
        {
            int idx = index % (g_nodeList.size());
            auto it = g_nodeList.begin();
            advance(it, idx);
            dbgprintf("[DEBUG] %s: Routing to %s\n", __func__, (it->first).c_str());
            return it->first;
        }

        string GetIpForResAlloc(string addr) 
        {
            int colon = addr.find(":");
            return addr.substr(0, colon+1) + RES_ALLOC_PORT_STR; 
        }

        bool IsNodeAtIndexLeader(int index)
        {
            int idx = index % (g_nodeList.size());
            auto it = g_nodeList.begin();
            advance(it, idx);
            return (it->second.identity == LEADER);
        }

        void SetLBCommIp(string ip)
        {
            _lbCommIp = ip;
        }

        string GetResAllocIp()
        {
            return _resAllocIp;
        }

        void SetResAllocIp(string ip)
        {
            _resAllocIp = ip;
        }

        string GetLBCommIp()
        {
            return _lbCommIp;
        }

        string GetNodeIpFromIndex(int index)
        {
            int idx = index % (g_nodeList.size());
            auto it = g_nodeList.begin();
            advance(it, idx);
            string ipWithPort = it->first;
            int colon = ipWithPort.find(":");
            return ipWithPort.substr(0, colon); // ip without port
        }

        int GetPort(string ipWithPort)
        {
            int colon = ipWithPort.find(":");
            return stoi(ipWithPort.substr(colon + 1));
        }

        pair<string, string> GenerateKvAndRaftIp(int index)
        {
            vector<int> alivePorts;
            vector<int> deadKvPorts;
            vector<int> deadRaftPorts;
            string ip;
            string newKvIp;
            string newRaftIp;
            
            for (auto itr = g_nodeList.begin(); itr != g_nodeList.end(); itr++)
            {
                string raftIp = itr->first;
                string kvIp = itr->second.kvIp;
                if (itr->second.status == ALIVE)
                {
                    alivePorts.push_back(GetPort(raftIp));
                    alivePorts.push_back(GetPort(kvIp));
                }
                else
                {
                    deadRaftPorts.push_back(GetPort(raftIp));
                    deadKvPorts.push_back(GetPort(kvIp));
                } 
            }

            ip = GetNodeIpFromIndex(index); // without port

            // Use the dead ports, if any
            if (deadKvPorts.size() != 0 && deadRaftPorts.size() != 0)
            {
                newKvIp = ip + ":" + to_string(deadKvPorts[0]);
                newRaftIp = ip + ":" + to_string(deadRaftPorts[0]);
            }   
            // generate port by adding +1/+2 to the max port in use         
            else
            {
                sort(alivePorts.begin(), alivePorts.end(), greater<int>()); // desc order
                newKvIp = ip + ":" + to_string(alivePorts[0] + 1);
                newRaftIp = ip + ":" + to_string(alivePorts[0] + 2);
            }

            dbgprintf("[DEBUG] %s: newKvIp = %s | newRaftIp = %s\n", __func__, newKvIp.c_str(), newRaftIp.c_str());
            return make_pair(newKvIp, newRaftIp);
        }
};


Helper _helper; // TODO: Make better

/******************************************************************************
 * DECLARATION
 *****************************************************************************/
/*
*   @brief Replays client requests to the server node(s)
*/
class KeyValueService final : public KeyValueOps::Service 
{
private:
    int _index = 0; 
    unordered_map<string, int> _valCountMap; // <value:frequency> map
    Helper _helper;

    /*
    *   @brief Gets server ip using round robin
    *
    *   @return Server stub
    */
   

    /* @brief get the stub for the leader ip if leader exists, else returns NULL
    * 
    * @return stub
    */
    KeyValueClient* getLeaderStub()
    {
        if (g_nodeList.count(g_leaderIP) == 0)
        {
            cout << "[WARN] No leader in the system yet" << endl;
            return NULL;
        }
        return g_nodeList[g_leaderIP].kvStub;
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
        if (consistencyLevel == STRONG_LEADER)
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

        else if (consistencyLevel == STRONG_MAJORITY)
        {
            // iterate over g_nodeList
            for(auto& node: g_nodeList) 
            {
                std::thread(&KeyValueService::invokeGetFromDB, this, node.second.kvStub, request, reply).detach();
            }

            do{
                // wait for majority
            }
            while(!receivedMajorityValue());

            reply->set_value(getMajorityResp());
            return Status::OK;
        }
        else { 
            _index = _index + 1;
            string raftIp = _helper.GetServerIPToRouteTo(_index);
            KeyValueClient* stub = g_nodeList[raftIp].kvStub;
            return stub->GetFromDB(*request, reply);
        }    
    }

    /*
    *   @brief calculates the number of responses needed for majority
    *          
    *   @return majority count
    */
    int GetMajorityCount() // TODO: Move to util
    {
        int size = g_nodeList.size();
        return (size % 2 == 0) ? (size / 2) : (size / 2) + 1;
    }

    /*
    *   @brief checks if majority responses have been received
    *          
    *   @return 
    *       true : on receiving majority or if there is only one node in the cluster
    *       false: otherwise
    */
    bool receivedMajorityValue() // TODO: Move to util
    {
        // Leader is the only node alive
        if (g_nodeList.size() == 1) 
        {
            return true;
        }

        int count = 0;
        
        for (auto& it: _valCountMap) {
            int count = it.second;
            if(count >= GetMajorityCount())
            {
                return true; // break on receiving majority
            }
        }
        return false;
    }

    /*
    *   @brief sends GetFromDB to specified stub,
    *           and updates valCountMap on receiving reply
    * 
    *   @param stub
    *   @param request
    *   @param reply
    */
    void invokeGetFromDB(KeyValueClient* stub, const GetRequest* request, GetReply* reply)
    {
        Status status = stub->GetFromDB(*request, reply);
        if (status.ok())
        {
            string value = reply->value();
            int count = _valCountMap[value];
            _valCountMap[value] = count + 1;
        }
    }

    /*
    *   @brief returns the value received from majority of g_nodeList
    * 
    *   @return value
    */
    string getMajorityResp() {
        int max = 0;
        string value;

        for(auto valCount: _valCountMap)
        {
            if(valCount.second > max) 
            {
                max = valCount.second;
                value = valCount.first;
            }
        }

        return value;
    }
    
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
        dbgprintf("g_leaderIP = %s\n", g_leaderIP.c_str());
        
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
    int _index; // for round-robin on add node
    /*
    *   @brief Save server node information
    *
    *   @param identity  
    *   @param ip 
    */
    void registerNode(int identity, string kvIp, string raftIp) 
    {
        _helper.SetNodeMetadata(identity, kvIp, raftIp);
        dbgprintf("[DEBUG]: Length of g_nodeList at LB: %ld\n", g_nodeList.size());
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
            _helper.SetNodeMetadata(identity, kvIp, raftIp);
        }
        else
        {
            g_nodeList[raftIp].identity = identity;
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
            nodeData->set_identity(it.second.identity);
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

    int getCountOfLiveNodes()
    {
        int count = 0;
        for (auto itr = g_nodeList.begin(); itr != g_nodeList.end(); itr++)
        {
            if (itr->second.status == ALIVE)
            {
                count++;
            }
        }

        return count;
    }

    // TODO test
    void invokeAddServer()
    {
        ClientContext addServercontext;
        AddServerRequest addServerRequest;
        AddServerReply addServerReply;
        
        dbgprintf("[DEBUG] %s: res alloc ip = %s\n", __func__, _helper.GetResAllocIp().c_str());

        auto resAllocStub = ResAlloc::NewStub(grpc::CreateChannel(string(_helper.GetResAllocIp()), grpc::InsecureChannelCredentials()));
        Status addServerStatus = resAllocStub->AddServer(&addServercontext, addServerRequest, &addServerReply);
        cout << "status = " << addServerStatus.error_code() << endl;
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

            if (!shouldAutoScale && g_nodeList.size() == AUTO_SCALE_THRESHOLD)
            {
                shouldAutoScale = true;
            }
        }

        cout << "[ERROR]: stream broke" << endl;
        // Do not delete node
        g_nodeList[raftIp].status = DEAD;
        if (getCountOfLiveNodes() < AUTO_SCALE_THRESHOLD && shouldAutoScale)
        {
            cout << "[INFO] Autoscale" << endl;
            invokeAddServer();
        }
        
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
    Status AssertLeadership(ServerContext* context, const AssertLeadershipRequest* request, AssertLeadershipReply* reply) override
    {
        dbgprintf("[DEBUG] %s: Entering function\n", __func__);
        dbgprintf("[DEBUG] %s: Previous g_leaderIP = %s\n", __func__, g_leaderIP.c_str());
        string leaderRaftIp;
        string leaderKvIp;
        // Only update the leader if it is from the right term
        if (g_currentTerm < request->term())
        {
            g_currentTerm = request->term();

            // set previous leader to follower
            if (g_nodeList.count(g_leaderIP) != 0)
            {
                g_nodeList[g_leaderIP].identity = FOLLOWER;
            }

            // set new leader
            leaderRaftIp = request->leader_raft_ip();
            leaderKvIp = request->leader_kv_ip();

            g_leaderIP = leaderRaftIp; // set global leader ip 

            dbgprintf("[DEBUG] %s: New g_leaderIP = %s\n", __func__, g_leaderIP.c_str());
            if (g_nodeList.count(g_leaderIP) == 0)
            {
                _helper.SetNodeMetadata(LEADER, leaderKvIp, leaderRaftIp);
            }
            else
            {
                g_nodeList[leaderRaftIp].identity = LEADER;
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
 * DECLARATION
 *****************************************************************************/
class ResAllocRelayService final: public ResAlloc::Service
{
private:
    int _index = 0;
   
public:
    /*
    *   @brief adds another server by calling resAlloc stub
    *
    *   @param context
    *   @param request 
    *   @param reply
    * 
    *   @return gRPC status
    */
    Status AddServer(ServerContext* context, const AddServerRequest* clientRequest, AddServerReply* clientReply) override 
    {
        // get res alloc stub
        string raftIp = _helper.GetServerIPToRouteTo(_index);  
        string resAllocIP = _helper.GetIpForResAlloc(raftIp);
        auto stub = ResAlloc::NewStub(grpc::CreateChannel(resAllocIP, grpc::InsecureChannelCredentials()));
        
        _index = _index + 1; 

        // request
        ClientContext clientContext;
        AddServerRequest request;
        AddServerReply reply;
        auto newIps = _helper.GenerateKvAndRaftIp(_index);
        request.set_kv_ip(newIps.first); 
        request.set_raft_ip(newIps.second);
        request.set_lb_ip(_helper.GetLBCommIp());

        dbgprintf("[DEBUG] %s: kvip = %s | raft ip = %s | lb ip = %s\n", __func__, request.kv_ip().c_str(), request.raft_ip().c_str(), request.lb_ip().c_str());

        return stub->AddServer(&clientContext, request, &reply);
    }

    /*
    *   @brief deletes a server by calling resAlloc stub
    *
    *   @param context
    *   @param request 
    *   @param reply
    * 
    *   @return gRPC status
    */
    Status DeleteServer(ServerContext* context, const DeleteServerRequest* clientRequest, DeleteServerReply* clientReply) override 
    {
        dbgprintf("[DEBUG] %s: Inside function\n", __func__);
        if(_helper.IsNodeAtIndexLeader(_index))
        {
            _index = _index + 1; 
        }

        // get res alloc stub
        string raftIp = _helper.GetServerIPToRouteTo(_index);   
        string resAllocIP = _helper.GetIpForResAlloc(raftIp);
        auto stub = ResAlloc::NewStub(grpc::CreateChannel(resAllocIP, grpc::InsecureChannelCredentials()));
        
        _index = _index + 1; 

        // request
        ClientContext clientContext;
        DeleteServerRequest request;
        DeleteServerReply reply;
        request.set_kv_ip(g_nodeList[raftIp].kvIp);
        request.set_raft_ip(raftIp);
        dbgprintf("[DEBUG] %s: kv ip = %s | raft ip = %s\n", __func__, request.kv_ip().c_str(), request.raft_ip().c_str());

        return stub->DeleteServer(&clientContext, request, &reply);
    }
};

void* RunResAllocRelayService(void* args)
{
    string server_address((char *) args);
    ResAllocRelayService service;
    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    unique_ptr<Server> server(builder.BuildAndStart());
    cout << "LB for ResAllocRelayServicelistening on " << server_address << std::endl;

    server->Wait();

    return NULL;
}


/******************************************************************************
 * DRIVER
 *****************************************************************************/
// @usage: ./loadbalancer <ip with port for kv client> <ip with port for servers> <ip with port for resalloc client>
int main (int argc, char *argv[]){
    // Init
    _helper.SetLBCommIp(argv[2]);
    _helper.SetResAllocIp(argv[3]);

    pthread_t client_server_t, node_server_t, resalloc_server_t;
  
    pthread_create(&client_server_t, NULL, RunServerForClient, argv[1]);
    pthread_create(&node_server_t, NULL, RunServerForNodes, argv[2]);
    pthread_create(&resalloc_server_t, NULL, RunResAllocRelayService, argv[3]);

    pthread_join(client_server_t, NULL);
    pthread_join(node_server_t, NULL);
    pthread_join(resalloc_server_t, NULL);
    
    return 0;
}