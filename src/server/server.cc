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
#include <thread>
#include <atomic>
#include <csignal>
#include <sys/time.h>
#include <cerrno>
#include <ctime>
#include <grpc++/grpc++.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>
#include "keyvalueops.grpc.pb.h"
#include "lb.grpc.pb.h"
#include "raft.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include "lb.grpc.pb.h"
#include "key_value_server.h"
#include "lb_comm_server.h"
#include "../util/common.h"
#include "../util/state_helper.h"
#include "raft_server.h"
#include "../util/levelDBWrapper.h"
#include <grpcpp/resource_quota.h>
/******************************************************************************
 * NAMESPACES
 *****************************************************************************/
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::ServerReader;
using grpc::Status;
using grpc::StatusCode;
using grpc::Service;
using grpc::ClientContext;
using grpc::ClientReaderWriter;
using namespace kvstore;
using namespace std;

/******************************************************************************
 * MACROS
 *****************************************************************************/

/******************************************************************************
 * GLOBALS
 *****************************************************************************/
StateHelper g_stateHelper;
RaftServer serverImpl;
unordered_map <string, pair<int, unique_ptr<Raft::Stub>>> g_nodeList;
RaftServer* signalHandlerService;
LBNodeCommClient* lBNodeCommClient; 

// for debug
void PrintNodesInNodeList()
{
    for (auto &item: g_nodeList)
    {
        dbgprintf("[DEBUG]: ip = %s| identity = %d\n", item.first.c_str(), item.second.first);
    }
}

string convertToLocalAddress(string addr) 
{
  int colon = addr.find(":");
  return "0.0.0.0:" + addr.substr(colon+1); 
}

string addToPort(string addr) 
{
  int colon = addr.find(":");
  int port  = stoi(addr.substr(colon+1)) + 1;
  return "0.0.0.0:" + to_string(port); 
}


string getRaftIp(string kvServerIp)
{
    int colon = kvServerIp.find(":");
    int port  = stoi(kvServerIp.substr(colon+1)) + 1;
    string raftIp = kvServerIp.substr(0,colon + 1) + to_string(port);
    dbgprintf("%s\n", raftIp.c_str());
    return  raftIp;
}

/******************************************************************************
 * DECLARATION: KeyValueOpsServiceImpl
 *****************************************************************************/
// TODO: use leveldb
grpc::Status KeyValueOpsServiceImpl::GetFromDB(ServerContext* context, const GetRequest* request, GetReply* reply) 
{
    cout << "[INFO] Received Get request" << endl;
    dbgprintf("[DEBUG] %s: Entering function\n", __func__);
    dbgprintf("[DEBUG] %s: Exiting function\n", __func__);
    return grpc::Status::OK;
}

grpc::Status KeyValueOpsServiceImpl::PutToDB(ServerContext* context,const PutRequest* request, PutReply* reply)  
{
    cout << "[INFO] Received Put request" << endl;
    dbgprintf("[DEBUG] %s: Entering function\n", __func__);

    serverImpl.ClearAppendEntriesMap();
            
    g_stateHelper.Append(g_stateHelper.GetCurrentTerm(), request->key(), request->value());        
    serverImpl.BroadcastAppendEntries();
            
    // wait for majority
    do 
    {
        dbgprintf("[DEBUG] %s: Waiting for majority\n", __func__);
        sleep(1);
    } while(!serverImpl.ReceivedMajority());
            
    g_stateHelper.SetCommitIndex(g_stateHelper.GetLogLength()-1);
    serverImpl.ExecuteCommands(g_stateHelper.GetLastAppliedIndex() + 1, g_stateHelper.GetCommitIndex());
            
    dbgprintf("[DEBUG] %s: Exiting function\n", __func__);
    return grpc::Status::OK;
}

void RunKeyValueServer(string args) 
{
    string ip = string(args);
    ip = convertToLocalAddress(ip);

    KeyValueOpsServiceImpl service;
    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    builder.AddListeningPort(ip, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    unique_ptr<Server> server(builder.BuildAndStart());
    
    cout << "[INFO] KeyValue Server listening on "<< ip << endl;
    
    server->Wait();
}

/******************************************************************************
 * DECLARATION: LBNodeCommClient
 *****************************************************************************/

/*
*   @brief Updates the global nodeList with the sys state received from the LB
* 
*   @param AssertLeadershipReply    
*/
void LBNodeCommClient::updateFollowersInNodeList(AssertLeadershipReply *reply)
{
    dbgprintf("[DEBUG] %s: Entering function\n", __func__);
    for (int i = 0; i < reply->follower_ip_size(); i++)
    {
        auto nodeData = reply->follower_ip(i);
        string ip = nodeData.ip();
        dbgprintf("[DEBUG] %s: ip = %s\n", __func__, ip.c_str());
        // new node
        if (g_nodeList.count(ip) == 0)
        {
            int leaderLastIndex = g_stateHelper.GetLogLength();
            g_stateHelper.SetNextIndex(ip, leaderLastIndex);
            g_stateHelper.SetMatchIndex(ip, leaderLastIndex);
            g_nodeList[ip] = make_pair(FOLLOWER, 
                                            Raft::NewStub(grpc::CreateChannel(ip, grpc::InsecureChannelCredentials())));
        }
        // old node, update identity
        else
        {
            g_nodeList[ip].first = FOLLOWER;
        }

        // TODECIDE: Should we delete nodes?
    }
    dbgprintf("[DEBUG] %s: Exiting function\n", __func__);
}

/*
*   @brief Created a stub to communicate with the LB
*/
LBNodeCommClient::LBNodeCommClient(string _lb_addr) 
{
    stub_ = LBNodeComm::NewStub(grpc::CreateChannel(_lb_addr, grpc::InsecureChannelCredentials()));
}

/*
*   @brief Sends periodic heartbeats to the LB,
*          updates the nodeList with the sys state received from the LB
*/
void LBNodeCommClient::SendHeartBeat() 
{
    ClientContext context;
            
    shared_ptr<ClientReaderWriter<HeartBeatRequest, HeartBeatReply> > stream(
                (LBNodeCommClient::stub_)->SendHeartBeat(&context));

    HeartBeatReply reply;
    HeartBeatRequest request;
    request.set_ip(serverImpl.GetMyIp());

    while(1) 
    {
        request.set_identity(g_stateHelper.GetIdentity());
        stream->Write(request);
        // dbgprintf("INFO] SendHeartBeat: sent heartbeat\n");

        stream->Read(&reply);
        // dbgprintf("[INFO] SendHeartBeat: recv heartbeat response\n");
                
        serverImpl.BuildSystemStateFromHBReply(reply);

        // dbgprintf("[INFO] SendHeartBeat: sleeping for 5 sec\n");
        sleep(HB_SLEEP_IN_SEC);
    }
}

/*
*   @brief Informs the LB of its identity as the new leader for current term
*          updates the nodeList with the sys state received from the LB
*/
void LBNodeCommClient::InvokeAssertLeadership()
{
    dbgprintf("[DEBUG] %s: Entering function\n", __func__);
    AssertLeadershipRequest request;
    AssertLeadershipReply reply;
    grpc::Status status;
    int retryCount = 0;

    request.set_term(g_stateHelper.GetCurrentTerm());
    request.set_leader_ip(serverImpl.GetMyIp());

    do
    {
        ClientContext context;
        reply.Clear();            
                
        status = stub_->AssertLeadership(&context, request, &reply);
        dbgprintf("[DEBUG]: status code = %d\n", status.error_code());
        dbgprintf("[DEBUG]: status message = %s\n", status.error_message().c_str());
        retryCount++;
        sleep(RETRY_TIME_START * retryCount * RETRY_TIME_MULTIPLIER);

    } while (status.error_code() == StatusCode::UNAVAILABLE);

    updateFollowersInNodeList(&reply);
    dbgprintf("[DEBUG] %s: Exiting function\n", __func__);
}

/*
*   @brief Creates a new client to communicate with LB and sends heartbeats
*
*   @param args to be used as lb address 
*/
void StartHB(string args) 
{
    string lb_addr = string(args);
    dbgprintf("[DEBUG] lb_addr = %s\n", lb_addr.c_str());
    lBNodeCommClient = new LBNodeCommClient(lb_addr); 
    lBNodeCommClient->SendHeartBeat();
}

/******************************************************************************
 * DECLARATION: Raft
 *****************************************************************************/

/*
*   @brief Sets self IP
*
*   @param ip
*/
void RaftServer::SetMyIp(string ip)
{
    _myIp = ip;
}

/*
*   @brief Gets self IP
*
*   @return ip
*/
string RaftServer::GetMyIp()
{
    return _myIp;
}

/*
*   @brief triggers AlarmCallBack 
*
*   @param signum
*/
void signalHandler(int signum) {
    signalHandlerService->AlarmCallback();
	return;
}

/*
*   @brief initializes the Raft Server,
*          updates global state with currentTerm, commitIndex, lastAppliedIndex,
*          resets election timeout with a random time duration,
*          prepares to trigger alarm at the end of election timeout
*/
void RaftServer::ServerInit() 
{    
    dbgprintf("[DEBUG]: %s: Inside function\n", __func__);
    g_stateHelper.SetIdentity(ServerIdentity::FOLLOWER);    
    
    
    int old_errno = errno;
    errno = 0;
    signal(SIGALRM, &signalHandler);
    
    if (errno) 
    {
        dbgprintf("ERROR] Signal could not be set\n");
        errno = old_errno;
        return;
    }

    signalHandlerService = this;

    errno = old_errno;
    srand(getpid());
    resetElectionTimeout();
    setAlarm(_electionTimeout);

    dbgprintf("[DEBUG]: %s: Exiting function\n", __func__);
}

/*
*   @brief builds cluster state based on HB reply from LB
*          
*   @param reply
*/
void RaftServer::BuildSystemStateFromHBReply(HeartBeatReply reply) 
{
    dbgprintf("[DEBUG] node size: %d \n", reply.node_data_size());
    for (int i = 0; i < reply.node_data_size(); i++)
    {
        auto nodeData = reply.node_data(i);
        dbgprintf("[DEBUG] nodeData.ip(): %s \n", nodeData.ip().c_str());

        if(g_nodeList.count(nodeData.ip()) == 0)
        {
            if(g_stateHelper.GetIdentity() == LEADER)
            {
                int leaderLastIndex = g_stateHelper.GetLogLength() - 1;
                g_stateHelper.SetNextIndex(nodeData.ip(), leaderLastIndex + 1);
                g_stateHelper.SetMatchIndex(nodeData.ip(), leaderLastIndex);
            }


            g_nodeList[nodeData.ip()] = make_pair(nodeData.identity(), 
                                            Raft::NewStub(grpc::CreateChannel(getRaftIp(nodeData.ip()), grpc::InsecureChannelCredentials())));
        }
        else
        {
            g_nodeList[nodeData.ip()].first = nodeData.identity();
        }
    }
}

/*
*   @brief calculates the number of responses needed for majority
*          
*   @return majority count
*/
int RaftServer::GetMajorityCount()
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
bool RaftServer::ReceivedMajority() 
{
    // Leader is the only node alive
    if (g_nodeList.size() == 1) 
    {
        return true;
    }

    int countSuccess = 1;
    
    for (auto& it: _appendEntriesResponseMap) {
        AppendEntriesReply replyReceived = it.second;
        if(replyReceived.success()) 
        {
            countSuccess++;
            if(countSuccess >= GetMajorityCount())
            {
                return true; // break on receiving majority
            }
        }
    }
    return false;
}

/*
*   @brief clears AppendEntriesMap          
*/
void RaftServer::ClearAppendEntriesMap() 
{
    _appendEntriesResponseMap.clear();
}

/*
*   @brief  candidate starts a new election,
*           resets election timeout,
*           calls invokeRequestVote() parallely for each node in the cluster,
*           becomes leader on getting majority
*/
void RaftServer::runForElection() 
{
    g_stateHelper.AddCurrentTerm(g_stateHelper.GetCurrentTerm() + 1);
    dbgprintf("[DEBUG] %s: Current term = %d\n", __func__, g_stateHelper.GetCurrentTerm());

    /* Vote for self - hence 1*/
    _votesGained = 1;
    g_stateHelper.AddVotedFor(g_stateHelper.GetCurrentTerm(), GetMyIp());
    dbgprintf("[DEBUG] %s: Voted for = %s\n", __func__, g_stateHelper.GetVotedFor(g_stateHelper.GetCurrentTerm()).c_str());

    /*Reset Election Timer*/
    resetElectionTimeout();
    setAlarm(_electionTimeout);

    /* Send RequestVote RPCs to all servers */
    for (auto& node: g_nodeList) {
        if (node.first != GetMyIp()) {
            
	 std::thread(&RaftServer::invokeRequestVote, this, node.first).detach();
        // TODO: try without threads
	//invokeRequestVote(node.first);
	}
    }

    dbgprintf("[DEBUG] %s: Going to sleep for 2 seconds\n", __func__);    
    sleep(2);
    //dbgprintf("[DEBUG] %s: _votesGained\n", __func__, _votesGained);    

    if (_votesGained > g_nodeList.size()/2 && g_stateHelper.GetIdentity() == ServerIdentity::CANDIDATE) {
        cout<<"[INFO] Candidate received majority of "<<_votesGained<<endl;
        cout<<"[INFO] Change Role to LEADER for term "<<g_stateHelper.GetCurrentTerm()<<endl;
        becomeLeader();
    }
    dbgprintf("[DEBUG] %s: Did not get enough votes in time\n", __func__); 
}

/*
*   @brief  sends RequestVote RPC to a node
*
*   @param nodeIp
*/       
void RaftServer::invokeRequestVote(string nodeIp) 
{

    cout << "[INFO]: Sending Request Vote to " << nodeIp << " | term = " << g_stateHelper.GetCurrentTerm() << endl;
    if(g_nodeList[nodeIp].second.get()==nullptr)
    {
       if (g_stateHelper.GetIdentity() == CANDIDATE)
       { 
            g_nodeList[nodeIp].second = Raft::NewStub(grpc::CreateChannel(getRaftIp(nodeIp), grpc::InsecureChannelCredentials()));
       }
       else
       {
	        return; // try	
       }
    }

    if(g_stateHelper.GetIdentity() == CANDIDATE && requestVote(g_nodeList[nodeIp].second.get()))
    {
        _votesGained++;
    }
}

/*
*   @brief  builds request for AppendEntries RPC
*
*   @param followerip, nextIndex
*/  
AppendEntriesRequest RaftServer::prepareRequestForAppendEntries (string followerip, int nextIndex) 
{
    dbgprintf("[DEBUG] %s: Entering function with nextIndex = %d\n", __func__, nextIndex);
    AppendEntriesRequest request;

    int logLength = g_stateHelper.GetLogLength();
    int prevLogIndex = g_stateHelper.GetMatchIndex(followerip);
    int prevLogTerm = g_stateHelper.GetTermAtIndex(prevLogIndex);

    request.set_term(g_stateHelper.GetCurrentTerm()); 
    request.set_leader_id(_myIp); 
    request.set_prev_log_index(prevLogIndex); 
    request.set_prev_log_term(prevLogTerm);
    request.set_leader_commit_index(g_stateHelper.GetCommitIndex());

    for(int i = nextIndex; i < logLength; i++) 
    {
        int term = g_stateHelper.GetTermAtIndex(i);
        string key = g_stateHelper.GetKeyAtIndex(i);
        string value = g_stateHelper.GetValueAtIndex(i);

        auto data = request.add_log_entry();
        data->set_term(term); 
        data->set_key(key); 
        data->set_value(value);
    }

    dbgprintf("[DEBUG] %s: logLength = %d | prevLogIndex = %d | prevLogTerm = %d | nextIndex = %d | Entries size = %d \n", __func__, logLength, prevLogIndex, prevLogTerm, nextIndex, request.log_entry_size());

    dbgprintf("[DEBUG] %s: Exiting function\n", __func__);
    return request;
}

/*
*   @brief  send AppendEntries RPC to follower,
*           retry in case of log inconsistencies,
*           become follower if reply contains a greater term
*
*   @param followerip
*/         
void RaftServer::invokeAppendEntries(string followerIp) 
{
    dbgprintf("[DEBUG] %s: Entering function with followerIp = %s\n", __func__, followerIp.c_str());

    if(g_stateHelper.GetIdentity() != LEADER)
    {
        return;
    }
    
    // Init params to invoke the RPC
    AppendEntriesRequest request;
    AppendEntriesReply reply;
    grpc::Status status;
    int nextIndex = 0;
    int matchIndex = 0;
    int retryCount = 0;
    bool shouldRetry = false;

    nextIndex = g_stateHelper.GetNextIndex(followerIp);
    matchIndex = g_stateHelper.GetMatchIndex(followerIp);

    // Retry the RPC until log is consistent
    do 
    {
        request = prepareRequestForAppendEntries(followerIp, nextIndex);
        dbgprintf("[DEBUG] %s: Checking if request is intact. Leader IP = %s\n", __func__, request.leader_id().c_str());
        auto stub = g_nodeList[followerIp].second.get();
        // Retry RPC indefinitely if follower is down
        retryCount = 0;
        do
        {
            ClientContext context;
            reply.Clear();            
            
            status = stub->AppendEntries(&context, request, &reply);
            cout << "[DEBUG] "<< __func__ <<" status code = " << status.error_code() << " | IP = " << followerIp <<endl;
            dbgprintf("[DEBUG]: status message = %s\n", status.error_message().c_str());
            retryCount++;
            sleep(RETRY_TIME_START * retryCount * RETRY_TIME_MULTIPLIER);

        } while (status.error_code() == StatusCode::UNAVAILABLE && g_stateHelper.GetIdentity() == LEADER);
              
        // Check if RPC should be retried because of log inconsistencies
        shouldRetry = (request.term() >= reply.term() && !reply.success() && status.error_code() == StatusCode::OK);
        dbgprintf("[DEBUG] %s: reply.term() = %d | reply.success() = %d\n", __func__, reply.term(), reply.success());
        dbgprintf("[DEBUG] %s: shouldRetry = %d\n", __func__, shouldRetry);
        // AppendEntries failed because of log inconsistencies
        if (shouldRetry) 
        {
            g_stateHelper.SetNextIndex(followerIp, nextIndex-1);
            g_stateHelper.SetMatchIndex(followerIp, matchIndex-1);
        }

    } while (shouldRetry && g_stateHelper.GetIdentity() == LEADER);

    // Leader becomes follower
    if (request.term() < reply.term())
    {  
        becomeFollower();
    }
    // RPC succeeded on the follower - Update match index and next index
    else if(reply.success())
    {
        dbgprintf("[DEBUG] %s: RPC sucess\n", __func__);
        g_stateHelper.SetMatchIndex(followerIp, g_stateHelper.GetLogLength()-1);
        g_stateHelper.SetNextIndex(followerIp, g_stateHelper.GetLogLength());
    }

    _appendEntriesResponseMap[followerIp] = reply;

    dbgprintf("[DEBUG] %s: Exiting function\n", __func__);
}

/*
*   @brief  send requestVote RPC to follower node stub
*           
*   @param stub
*/ 
bool RaftServer::requestVote(Raft::Stub* stub) {
    dbgprintf("[DEBUG] %s: In function\n", __func__);
    ReqVoteRequest req;
    ReqVoteReply reply;
    ClientContext context;

    dbgprintf("[DEBUG] %s: Log Length %d = \n", __func__, g_stateHelper.GetLogLength());
    req.set_term(g_stateHelper.GetCurrentTerm());
    req.set_candidate_id(_myIp);
    req.set_last_log_index(g_stateHelper.GetLogLength()-1);
    req.set_last_log_term(g_stateHelper.GetTermAtIndex(g_stateHelper.GetLogLength()-1));
    dbgprintf("[DEBUG]: Send ReqVote with param | term = %d | candidate_id = %s | last_log_index = %d | last_log_term = %d\n", req.term(), req.candidate_id().c_str(), req.last_log_index(), req.last_log_term());
    
    grpc::Status status = stub->ReqVote(&context, req, &reply);
    cout << "[DEBUG] " << __func__ << " status code = " << status.error_code() << endl;

    if(status.ok() && reply.vote_granted_for())
    {   
        return true;
    }
    
    if(status.ok() && reply.term() > g_stateHelper.GetCurrentTerm())
    {
        becomeFollower();
    }
    return false;
}


/*
*   @brief  Leader broadcasts  by creating threads for each node,
*           stops if it learns it is no longer the leader
*/ 
void RaftServer::BroadcastAppendEntries() 
{
    dbgprintf("[DEBUG] %s: Entering function\n", __func__);
    for (auto& node: g_nodeList) 
    {
        if (node.first != _myIp) 
        {
            thread(&RaftServer::invokeAppendEntries, this, node.first).detach();
        }
    }
    
    // Stop this function if leader learns that it no longer is the leader
    if (g_stateHelper.GetIdentity() == ServerIdentity::FOLLOWER)
    {
        dbgprintf("[DEBUG] %s: Leader becomes follower\n", __func__);
        return;
    }
    dbgprintf("[DEBUG] %s: Exiting function\n", __func__);
}

/*
*   @brief  Leader sets its identity as leader,
*           matchIndex and nextIndex to its last index,
*           invokes AssertLeadership RPC to inform LB of its leadership,
*           spawns a thead to periodically call AppendEntries RPC,
*/       
void RaftServer::becomeLeader() 
{
    dbgprintf("[DEBUG] %s: Entering function\n", __func__);

    g_stateHelper.SetIdentity(ServerIdentity::LEADER);

    setNextIndexToLeaderLastIndex();
    setMatchIndexToLeaderLastIndex();

    // inform LB
    lBNodeCommClient->InvokeAssertLeadership();

    // to maintain leadership
    thread(&RaftServer::invokePeriodicAppendEntries, this).detach();

    dbgprintf("[DEBUG] %s: Exiting function\n", __func__);
}

/*
*   @brief  BroadCast AppendEntries periodically
*/ 
void RaftServer::invokePeriodicAppendEntries()
{
    while (g_stateHelper.GetIdentity() == LEADER)
    {
        dbgprintf("[INFO] %s: Raising periodic Append Entries\n", __func__);
        BroadcastAppendEntries();
        sleep(HB_SLEEP_IN_SEC);
    }
}

/*
*   @brief sets nextIndex to last index on leader's log
*/ 
// TODO: Change function name to Leader Log length
void RaftServer::setNextIndexToLeaderLastIndex() 
{
    int leaderLastIndex = g_stateHelper.GetLogLength() - 1;

    for(auto& node: g_nodeList) 
    {
        g_stateHelper.SetNextIndex(node.first, leaderLastIndex + 1);
        dbgprintf("[DEBUG] Next index: IP = %s | Index = %d\n", node.first.c_str(), leaderLastIndex);
    }
}

/*
*   @brief   sets matchIndex to last index on leader's log
*/ 
void RaftServer::setMatchIndexToLeaderLastIndex() 
{
    int leaderLastIndex = g_stateHelper.GetLogLength() - 1;
    
    for(auto& node: g_nodeList) 
    {
        g_stateHelper.SetMatchIndex(node.first, leaderLastIndex);
        dbgprintf("[DEBUG] Match index: IP = %s | Index = %d\n", node.first.c_str(), leaderLastIndex);
    }
}

/*
*   @brief   sets identity of node to follower
*/
void RaftServer::becomeFollower() 
{
    g_stateHelper.SetIdentity(ServerIdentity::FOLLOWER);
}
        
/* 
*    @brief     set identity as candidate, 
*               resets election timeout,
*               runs for election on timeout 
*/
void RaftServer::becomeCandidate() 
{
    g_stateHelper.SetIdentity(ServerIdentity::CANDIDATE);
    dbgprintf("[INFO] Become Candidate\n");
    runForElection();
}

/* 
*    @brief     
*/
void RaftServer::AlarmCallback() {
  if (g_stateHelper.GetIdentity() == ServerIdentity::LEADER) {
    // do nothing
  } else {
    becomeCandidate();
  }
}

/* 
*    @brief   sets election timeout to a random duration   
*/
void RaftServer::resetElectionTimeout() 
{
    _electionTimeout = _minElectionTimeout + (rand() % 
                                            _maxElectionTimeout - _minElectionTimeout + 1);
	dbgprintf("[DEBUG] %s: _electionTimeout = %d\n", __func__, _electionTimeout);
}

/* 
*    @brief   
*/
void RaftServer::setAlarm(int after_ms) {
    if (g_stateHelper.GetIdentity() == ServerIdentity::FOLLOWER) {
        // TODO:
    }

    struct itimerval timer;
    timer.it_value.tv_sec = after_ms / 1000;
    timer.it_value.tv_usec = 1000 * (after_ms % 1000);
    timer.it_interval = timer.it_value;

    int old_errno = errno;
    errno = 0;
    setitimer(ITIMER_REAL, &timer, nullptr);
    if(errno) {
        dbgprintf("INFO] Setting timer failed\n");
    }
    errno = old_errno;
    return;
}

/* 
*    @brief   On receiving AppendEntries RPC,   
*                   Case 1: Leader term < my term, reject the RPC
*                   Case 2: Candidate receives valid AppendEntries RPC
*                               a: in case of mismatch in term at log index , reject
*                               b. otherwise, apply entries to log and execute commands
*
*   @param context 
*   @param request 
*   @param response 
*
*   @return grpc Status
*/
grpc::Status RaftServer::AppendEntries(ServerContext* context, 
                                            const AppendEntriesRequest* request, 
                                            AppendEntriesReply *reply)
{
    dbgprintf("[DEBUG]: AppendEntries - Entering RPC\n");
    cout << "Append Entries RPC" << endl;
    int my_term = 0;

    // Case 1: leader term < my term
    my_term = g_stateHelper.GetCurrentTerm();
    if (request->term() < my_term)
    {
        // dbgprintf("[DEBUG]: AppendEntries RPC - leader term < my term\n");
        cout << "[APPEND ENTRIES]: Rejected - term is behind" << endl;
        reply->set_term(my_term);
        reply->set_success(false);
        return grpc::Status::OK;
    }

    // Case 2: Candidate receives valid AppendEntries RPC
    else 
    {
        // valid RPC
	    resetElectionTimeout();
    	setAlarm(_electionTimeout);

        if (g_stateHelper.GetIdentity() == ServerIdentity::CANDIDATE)
        {
            cout << "[APPEND ENTRIES]: Candidate becomes Follower" << endl;
            // dbgprintf("[DEBUG]: AppendEntries RPC - Candidate received a valid AppendEntriesRPC, becoming follower\n");
            becomeFollower();
        }

        // Check if term at log index matches
        if (g_stateHelper.GetTermAtIndex(request->prev_log_index()) != request->prev_log_term())
        {
            cout << "[APPEND ENTRIES]: Log inconsistent, AppendEntries will be retried" << endl;
            // dbgprintf("[DEBUG]: AppendEntries RPC - term mismatch at log index\n");
            reply->set_term(my_term);
            reply->set_success(false);
            return grpc::Status::OK;
        } 
        else 
        {   
            cout << "[APPEND ENTRIES]: Log consistent, adding entries if any" << endl;
            // dbgprintf("[DEBUG]: AppendEntries RPC - No log inconsistencies\n");
            // Apply entries to log
            vector<Entry> entries;
            for (int i = 0; i < request->log_entry_size(); i++) 
            {
                auto logEntry = request->log_entry(i);

                Entry entry(logEntry.term(),
                            logEntry.key(),
                            logEntry.value());

                entries.push_back(entry);
            }

            g_stateHelper.Insert(request->prev_log_index()+1, entries);

            // Execute commands
            if(request->leader_commit_index() > g_stateHelper.GetCommitIndex())
            {
                g_stateHelper.SetCommitIndex(request->leader_commit_index());

                ExecuteCommands(g_stateHelper.GetLastAppliedIndex(), request->leader_commit_index());
            }

            reply->set_term(my_term);
            reply->set_success(true);
            return grpc::Status::OK;
        }
    }

    dbgprintf("[DEBUG]: AppendEntries - Exiting RPC\n");
    return grpc::Status::OK;
}

/* 
*    @brief   Execute commands from start index to end index,
                set lastAppliedIndex iteratively
*
*   @param start 
*   @param end 
*
*/
void RaftServer::ExecuteCommands(int start, int end) 
{
    dbgprintf("[DEBUG] %s: Entering function\n", __func__);
    for(int i = start; i <= end; i++)
    {   // TODO: Uncomment after pulling cmake changes for leveldb from main 
        // levelDBWrapper.Put(g_stateHelper.GetKeyAtIndex(i), g_stateHelper.GetValueAtIndex(i));
        // TODO: check failure
        g_stateHelper.SetLastAppliedIndex(i); 
    }
    dbgprintf("[DEBUG] %s: Exiting function\n", __func__);
}

/* 
*   @brief   On receiving ReqVote RPC, 
*               Case 1: if req term < current term
*               Case 2: if req term > current term
                Case 3: if no votes have been made for this term, or voted for this leader before
*
*   @param context 
*   @param request 
*   @param response 
*
*   @return grpc Status
*
*/
grpc::Status RaftServer::ReqVote(ServerContext* context, const ReqVoteRequest* request, ReqVoteReply* reply) 
{
    cout<<"[ELECTION] Received Reqvote from " << request->candidate_id() << " for term " << request->term() << endl;
    int myCurrentTerm = g_stateHelper.GetCurrentTerm();
    int myLastLogIndex = g_stateHelper.GetLogLength() - 1;

    if(request->term() < myCurrentTerm)
    {
        reply->set_term(myCurrentTerm);
        reply->set_vote_granted_for(false);
        cout << "[ELECTION] Rejected Reqvote from " << request->candidate_id() << " for term " << request->term() << endl;
        return grpc::Status::OK;
    }
    else 
    {
        // update my term and become follower
        if(request->term() > g_stateHelper.GetCurrentTerm())
        {
            g_stateHelper.AddCurrentTerm(request->term()); 
            myCurrentTerm = g_stateHelper.GetCurrentTerm();
            becomeFollower();
        }

        // Check if you can vote for this term
        if(g_stateHelper.GetVotedFor(request->term()) == "")
        {
            // Candidate's term is more than mine
            if(request->last_log_term() > g_stateHelper.GetTermAtIndex(myLastLogIndex))
            {
                g_stateHelper.AddVotedFor(request->term(), request->candidate_id());
                reply->set_term(myCurrentTerm);
                reply->set_vote_granted_for(true);

                cout << "[ELECTION] Granted vote for " << request->candidate_id() << " for term " << request->term() << " Reason: Case A" << endl;
                return grpc::Status::OK;
            }
            else if(request->last_log_term() == g_stateHelper.GetTermAtIndex(myLastLogIndex))
            {
                // Candidate's term is same as mine & has equal/more logs than me
                if(request->last_log_index() >= myLastLogIndex)
                {
                    g_stateHelper.AddVotedFor(request->term(), request->candidate_id());
                    reply->set_term(myCurrentTerm);
                    reply->set_vote_granted_for(true);
        
                    cout << "[ELECTION] Granted Reqvote from " << request->candidate_id() << " for term " << request->term() << " Reason: Case B"<<endl;            
                    return grpc::Status::OK;
                }
                else
                {
                    reply->set_term(myCurrentTerm);
                    reply->set_vote_granted_for(false);
                    cout << "[ELECTION] Rejected Reqvote from " << request->candidate_id() << " for term " << request->term() << endl;
                    return grpc::Status::OK;
                }
            }
            else
            {
                reply->set_term(myCurrentTerm);
                reply->set_vote_granted_for(false);
                cout << "[ELECTION] Rejected Reqvote from " << request->candidate_id() << " for term " << request->term() << endl;
                return grpc::Status::OK;
            }
        }
        else
        {    
            reply->set_term(myCurrentTerm);
            reply->set_vote_granted_for(false);
            cout << "[ELECTION] Rejected Reqvote from " << request->candidate_id() << " for term " << request->term() << endl;
            return grpc::Status::OK;
        }
    }
    reply->set_term(myCurrentTerm);
    reply->set_vote_granted_for(false);
    cout << "[ELECTION] Rejected Reqvote from " << request->candidate_id() << " for term " << request->term() << endl;
    return grpc::Status::OK;
}

void RunRaftServer(string ip) {
    ip = addToPort(ip);
    dbgprintf("RunRaftServer listening on %s \n", ip.c_str());
    ServerBuilder builder;
    builder.AddListeningPort(ip, grpc::InsecureServerCredentials());
    builder.RegisterService(&serverImpl);
    unique_ptr<Server> server(builder.BuildAndStart());
    serverImpl.ServerInit();
    server->Wait();
}

/*
*   @usage: ./server <my ip with port>  <lb ip>
*/
int main(int argc, char **argv) 
{
    // init
    g_stateHelper = *(new StateHelper(argv[1]));
    g_stateHelper.SetIdentity(FOLLOWER);
    serverImpl.SetMyIp(argv[1]);
   
    // TODO: Uncomment later 
    std::thread(RunKeyValueServer, argv[1]).detach();
    std::thread(StartHB, argv[2]).detach();
    std::thread(RunRaftServer, argv[1]).detach();
   
    // Keep this loop, so that the program doesn't return
    while(1) {
    }

    return 0;
}
