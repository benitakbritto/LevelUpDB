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
// #include "../util/levelDBWrapper.h" // TODO: Build failing
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

/******************************************************************************
 * DECLARATION: KeyValueOpsServiceImpl
 *****************************************************************************/
// TODO: use leveldb
Status KeyValueOpsServiceImpl::GetFromDB(ServerContext* context, const GetRequest* request, GetReply* reply)  
{
    dbgprintf("[DEBUG] %s: Entering function\n", __func__);
    dbgprintf("[DEBUG] %s: Exiting function\n", __func__);
    return Status::OK;
}

Status KeyValueOpsServiceImpl::PutToDB(ServerContext* context,const PutRequest* request, PutReply* reply)  
{
    dbgprintf("[DEBUG] %s: Entering function\n", __func__);            
    serverImpl.ClearAppendEntriesMap();
            
    g_stateHelper.Append(g_stateHelper.GetCurrentTerm(), request->key(), request->value());        
    serverImpl.BroadcastAppendEntries();
            
    // wait for majority
    do 
    {
        // dbgprintf("[DEBUG] %s: Waiting for majority\n", __func__);
    } while(!serverImpl.ReceivedMajority());
            
    g_stateHelper.SetCommitIndex(g_stateHelper.GetLogLength()-1);
    serverImpl.ExecuteCommands(g_stateHelper.GetLastAppliedIndex() + 1, g_stateHelper.GetCommitIndex());
            
    dbgprintf("[DEBUG] %s: Exiting function\n", __func__);
    return Status::OK;
}

void *RunKeyValueServer(void* args) 
{
    string ip = string((char *) args);
    dbgprintf("[DEBUG]: %s: ip = %s\n", __func__, ip.c_str());

    KeyValueOpsServiceImpl service;
    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    builder.AddListeningPort(ip, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    unique_ptr<Server> server(builder.BuildAndStart());
    
    cout << "[INFO] KeyValue Server listening on "<< ip << endl;
    
    server->Wait();
    return NULL;
}

/******************************************************************************
 * DECLARATION: LBNodeCommClient
 *****************************************************************************/
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
  
LBNodeCommClient::LBNodeCommClient(string _lb_addr) 
{
    stub_ = LBNodeComm::NewStub(grpc::CreateChannel(_lb_addr, grpc::InsecureChannelCredentials()));
}

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
        dbgprintf("INFO] SendHeartBeat: sent heartbeat\n");

        stream->Read(&reply);
        dbgprintf("[INFO] SendHeartBeat: recv heartbeat response\n");
                
        // TODO : Parse reply to get sys state - is this done?
        serverImpl.BuildSystemStateFromHBReply(reply);

        dbgprintf("[INFO] SendHeartBeat: sleeping for 5 sec\n");
        sleep(HB_SLEEP_IN_SEC);
    }
}

void LBNodeCommClient::InvokeAssertLeadership()
{
    dbgprintf("[DEBUG] %s: Entering function\n", __func__);
    AssertLeadershipRequest request;
    AssertLeadershipReply reply;
    Status status;
    int retryCount = 0;

    request.set_leader_ip(serverImpl.GetMyIp());

    do
    {
        ClientContext context;
        reply.Clear();            
                
        status = stub_->AssertLeadership(&context, request, &reply);
        dbgprintf("[DEBUG]: status code = %d\n", status.error_code());
        retryCount++;
        sleep(RETRY_TIME_START * retryCount * RETRY_TIME_MULTIPLIER);

    } while (status.error_code() == StatusCode::UNAVAILABLE);

    updateFollowersInNodeList(&reply);
    dbgprintf("[DEBUG] %s: Exiting function\n", __func__);
}

void *StartHB(void* args) 
{
    string lb_addr = string((char *) args);
    dbgprintf("[DEBUG] lb_addr = %s\n", lb_addr.c_str());
    lBNodeCommClient = new LBNodeCommClient(lb_addr); 
    lBNodeCommClient->SendHeartBeat();

    return NULL;
}

/******************************************************************************
 * DECLARATION: Raft
 *****************************************************************************/
void RaftServer::SetMyIp(string ip)
{
    _myIp = ip;
}

string RaftServer::GetMyIp()
{
    return _myIp;
}

void signalHandler(int signum) {
    signalHandlerService->AlarmCallback();
	return;
}

// TODO: Fix param - is it fixed?
void RaftServer::ServerInit() 
{    
    dbgprintf("[DEBUG]: %s: Inside function\n", __func__);
    g_stateHelper.SetIdentity(ServerIdentity::FOLLOWER);    
    // hostList = {"0.0.0.0:50051", "0.0.0.0:50052", "0.0.0.0:50053"};

    //Initialize states
    g_stateHelper.AddCurrentTerm(0); // TODO @Shreyansh: need to read the term and not set to 0 at all times
    g_stateHelper.SetCommitIndex(0);
    g_stateHelper.SetLastAppliedIndex(0);
    // g_stateHelper.Append(0, "NULL", "NULL"); // To-Do: Need some initial data on file creation. Not a problem if file already exists.

    int old_errno = errno;
    errno = 0;
    cout<<"[INIT] Server Init"<<endl;
    signal(SIGALRM, &signalHandler);
    
    if (errno) 
    {
        dbgprintf("ERROR] Signal could not be set\n");
        errno = old_errno;
        return;
    }

    signalHandlerService = this;

    errno = old_errno;
    srand(time(NULL));
    resetElectionTimeout();
    setAlarm(_electionTimeout);

    dbgprintf("[DEBUG]: %s: Exiting function\n", __func__);
}

void RaftServer::BuildSystemStateFromHBReply(HeartBeatReply reply) 
{
    dbgprintf("[DEBUG] node size: %d \n", reply.node_data_size());
    for (int i = 0; i < reply.node_data_size(); i++)
    {
        auto nodeData = reply.node_data(i);
        g_nodeList[nodeData.ip()] = make_pair(nodeData.identity(), 
                                            Raft::NewStub(grpc::CreateChannel(nodeData.ip(), grpc::InsecureChannelCredentials())));
    }
}

int RaftServer::GetMajorityCount()
{
    int size = g_nodeList.size();
    return (size % 2 == 0) ? (size / 2) : (size / 2) + 1;
}

bool RaftServer::ReceivedMajority() 
{
    // Leader is the only node alive
    if (g_nodeList.size() == 1) 
    {
        return true;
    }

    int countSuccess = 0;
    
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

void RaftServer::ClearAppendEntriesMap() 
{
    _appendEntriesResponseMap.clear();
}

/* Candidate starts a new election */
void RaftServer::runForElection() 
{
    int initialTerm = g_stateHelper.GetCurrentTerm();
    g_stateHelper.AddCurrentTerm(g_stateHelper.GetCurrentTerm() + 1);

    /* Vote for self - hence 1*/
    _votesGained = 1;
    g_stateHelper.AddVotedFor(g_stateHelper.GetCurrentTerm(), GetMyIp());

    /*Reset Election Timer*/
    resetElectionTimeout();
    setAlarm(_electionTimeout);

    /* Send RequestVote RPCs to all servers */
    for (auto& node: g_nodeList) {
        if (node.first !=GetMyIp()) {
            std::thread(&RaftServer::invokeRequestVote, this, node.first).detach();
        }
    }
    
    sleep(2);
    //cout<<"Woken up: " <<_votesGained<<endl;
    if (_votesGained > g_nodeList.size()/2 && g_stateHelper.GetIdentity() == ServerIdentity::CANDIDATE) {
        cout<<"[INFO] Candidate received majority of "<<_votesGained<<endl;
        cout<<"[INFO] Change Role to LEADER for term "<<g_stateHelper.GetCurrentTerm()<<endl;
        becomeLeader();
    }
}
        
void RaftServer::invokeRequestVote(string host) {

    cout<<"[INFO]: Sending Request Vote to "<<host<<endl;
    if(g_nodeList[host].second.get()==nullptr)
    {
       g_nodeList[host].second = Raft::NewStub(grpc::CreateChannel(host, grpc::InsecureChannelCredentials()));
    }

    if(requestVote(g_nodeList[host].second.get()))
    {
        _votesGained++;
    }
}

AppendEntriesRequest RaftServer::prepareRequestForAppendEntries (string followerip, int nextIndex) 
{
    dbgprintf("[DEBUG] %s: Entering function with nextIndex = %d\n", __func__, nextIndex);
    AppendEntriesRequest request;

    int retryCount = 0;
    int logLength = g_stateHelper.GetLogLength();
    int prevLogIndex = g_stateHelper.GetMatchIndex(followerip);
    int prevLogTerm = g_stateHelper.GetTermAtIndex(prevLogIndex);

    request.set_term(g_stateHelper.GetCurrentTerm()); 
    request.set_leader_id(_myIp); 
    request.set_prev_log_index(prevLogIndex); 
    request.set_prev_log_term(prevLogTerm);
    request.set_leader_commit_index(g_stateHelper.GetCommitIndex());

    // TODO: Improve efficiency
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

    dbgprintf("[DEBUG] %s: Exiting function\n", __func__);
    return request;
}
        
void RaftServer::invokeAppendEntries(string followerIp) 
{
    dbgprintf("[DEBUG] %s: Entering function with followerIp = %s\n", __func__, followerIp.c_str());
    
    // Init params to invoke the RPC
    AppendEntriesRequest request;
    AppendEntriesReply reply;
    Status status;
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
        // TODO: Use nodeList data structure
        auto stub = Raft::NewStub(grpc::CreateChannel(followerIp, grpc::InsecureChannelCredentials()));

        // Retry RPC indefinitely if follower is down
        retryCount = 0;
        do
        {
            ClientContext context;
            reply.Clear();            
            
            status = stub->AppendEntries(&context, request, &reply);
            dbgprintf("[DEBUG]:  status code = %d\n", status.error_code());
            retryCount++;
            sleep(RETRY_TIME_START * retryCount * RETRY_TIME_MULTIPLIER);

        } while (status.error_code() == StatusCode::UNAVAILABLE);
      
        // Check if RPC should be retried because of log inconsistencies
        shouldRetry = (request.term() >= reply.term() && !reply.success());
        dbgprintf("[DEBUG] %s: reply.term() = %d | reply.success() = %d\n", __func__, reply.term(), reply.success());
        dbgprintf("[DEBUG] %s: shouldRetry = %d\n", __func__, shouldRetry);
        // AppendEntries failed because of log inconsistencies
        if (shouldRetry) 
        {
            g_stateHelper.SetNextIndex(followerIp, nextIndex-1);
            g_stateHelper.SetMatchIndex(followerIp, matchIndex-1);
        }

    } while (shouldRetry);

    // Leader becomes follower
    if (request.term() < reply.term())
    {  
        becomeFollower();
    }
    // RPC succeeded on the follower - Update match index
    else if(reply.success())
    {
        dbgprintf("[DEBUG] %s: RPC sucess\n", __func__);
        g_stateHelper.SetMatchIndex(followerIp, g_stateHelper.GetLogLength()-1);
    }

    dbgprintf("[DEBUG] %s: Exiting function\n", __func__);
}

bool RaftServer::requestVote(Raft::Stub* stub) {
    ReqVoteRequest req;

    req.set_term(g_stateHelper.GetCurrentTerm());
    req.set_candidateid(_myIp);
    req.set_lastlogindex(g_stateHelper.GetLogLength());
    cout<< g_stateHelper.GetTermAtIndex(g_stateHelper.GetLogLength()-1);
    req.set_lastlogterm(g_stateHelper.GetTermAtIndex(g_stateHelper.GetLogLength()-1));

    ReqVoteReply reply;
    ClientContext context;
    context.set_deadline(chrono::system_clock::now() + 
        chrono::milliseconds(_heartbeatInterval));

    grpc::Status status = stub->ReqVote(&context, req, &reply);

    setAlarm(_electionTimeout);

    if(status.ok() && reply.votegrantedfor())
        return true;
    
    if(status.ok() && reply.term()>g_stateHelper.GetCurrentTerm())
    {
        g_stateHelper.SetIdentity(ServerIdentity::FOLLOWER);
    }
    
    return false;
}

// Node calls this function after it becomes a leader  
void RaftServer::BroadcastAppendEntries() 
{
    dbgprintf("[DEBUG] %s: Entering function\n", __func__);
    dbgprintf("[DEBUG] %s: _nodeList.size() = %ld\n", __func__, g_nodeList.size());
    for (auto& node: g_nodeList) 
    {
        dbgprintf("[DEBUG]: ip of node %s \n", node.first.c_str());
        if (node.first != _myIp) 
        {
            dbgprintf("[DEBUG]: going to call invokeAppendEntries\n");
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

void RaftServer::invokePeriodicAppendEntries()
{
    while (1)
    {
        dbgprintf("[INFO] %s: Raising periodic Append Entries\n", __func__);
        BroadcastAppendEntries();
        sleep(HB_SLEEP_IN_SEC);
    }
}

void RaftServer::setNextIndexToLeaderLastIndex() 
{
    int leaderLastIndex = g_stateHelper.GetLogLength();

    for(auto& node: g_nodeList) 
    {
        g_stateHelper.SetNextIndex(node.first, leaderLastIndex);
    }
}

void RaftServer::setMatchIndexToLeaderLastIndex() 
{
    int leaderLastIndex = g_stateHelper.GetLogLength();
    
    for(auto& node: g_nodeList) 
    {
        g_stateHelper.SetMatchIndex(node.first, leaderLastIndex);
    }
}

void RaftServer::becomeFollower() 
{
    g_stateHelper.SetIdentity(ServerIdentity::FOLLOWER);
}
        
/* 
    Invoked when timeout signal is received - 
*/
void RaftServer::becomeCandidate() {
    g_stateHelper.SetIdentity(ServerIdentity::CANDIDATE);
    dbgprintf("INFO] Become Candidate\n");
    resetElectionTimeout();
    setAlarm(_electionTimeout);
    runForElection();
}

void RaftServer::AlarmCallback() {
  if (g_stateHelper.GetIdentity() == ServerIdentity::LEADER) {
    //ReplicateEntries();
  } else {
    becomeCandidate();
  }
}

void RaftServer::resetElectionTimeout() {
    _electionTimeout = _minElectionTimeout + (rand() % 
        (_maxElectionTimeout - _minElectionTimeout + 1));
}

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

Status RaftServer::AppendEntries(ServerContext* context, 
                                            const AppendEntriesRequest* request, 
                                            AppendEntriesReply *reply)
{
    dbgprintf("[DEBUG]: AppendEntries - Entering RPC\n");
    int my_term = 0;

    // Case 1: leader term < my term
    my_term = g_stateHelper.GetCurrentTerm();
    if (request->term() < my_term)
    {
        dbgprintf("[DEBUG]: AppendEntries RPC - leader term < my term\n");
        reply->set_term(my_term);
        reply->set_success(false);
        return Status::OK;
    }

    // Case 2: Candidate receives valid AppendEntries RPC
    else 
    {
        if (g_stateHelper.GetIdentity() == ServerIdentity::CANDIDATE)
        {
            dbgprintf("[DEBUG]: AppendEntries RPC - Candidate received a valid AppendEntriesRPC, becoming follower\n");
            becomeFollower();
        }

        // Check if term at log index matches
        if (g_stateHelper.GetTermAtIndex(request->prev_log_index()) != request->prev_log_term())
        {
            dbgprintf("[DEBUG]: AppendEntries RPC - term mismatch at log index\n");
            reply->set_term(my_term);
            reply->set_success(false);
            return Status::OK;
        } 
        else 
        {   
            dbgprintf("[DEBUG]: AppendEntries RPC - No log inconsistencies\n");
            
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
            return Status::OK;
        }
    }

    dbgprintf("[DEBUG]: AppendEntries - Exiting RPC\n");
    return Status::OK;
}


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

Status RaftServer::ReqVote(ServerContext* context, const ReqVoteRequest* request, ReqVoteReply* reply)
{
    cout<<"Received reqvote from "<<request->candidateid()<<" --- "<<request->term()<<endl;

    reply->set_votegrantedfor(false);
    reply->set_term(g_stateHelper.GetCurrentTerm());

    if(request->term() < g_stateHelper.GetCurrentTerm())
    {
        return grpc::Status::OK;
    }

    if(request->term() > g_stateHelper.GetCurrentTerm())
    {
        g_stateHelper.AddCurrentTerm(request->term());
        g_stateHelper.SetIdentity(ServerIdentity::FOLLOWER);
    }

    if(g_stateHelper.GetVotedFor(request->term())=="" || g_stateHelper.GetVotedFor(request->term()) == request->candidateid())
    {
        if(request->lastlogterm() > g_stateHelper.GetTermAtIndex(g_stateHelper.GetLogLength() - 1))
        {
            g_stateHelper.AddVotedFor(request->term(), request->candidateid());
            reply->set_votegrantedfor(true);

            return grpc::Status::OK;
        }

        if(request->lastlogterm() == g_stateHelper.GetTermAtIndex(g_stateHelper.GetLogLength() - 1))
        {
            if(request->lastlogindex() > g_stateHelper.GetLogLength()-1)
            {
                reply->set_votegrantedfor(true);

                return grpc::Status::OK;
            }
        }
    }

    return grpc::Status::OK;
}

void RunServer(string my_ip) {
    //string server_address("0.0.0.0:50051");

    /* TO-DO : Initialize GRPC connections to all other servers */
    serverImpl.SetMyIp(my_ip);
    ServerBuilder builder;
    // builder.SetMaxReceiveMessageSize((1.5 * 1024 * 1024 * 1024));
    builder.AddListeningPort(serverImpl.GetMyIp(), grpc::InsecureServerCredentials());
    builder.RegisterService(&serverImpl);
    unique_ptr<Server> server(builder.BuildAndStart());
	dbgprintf("[INFO] Server is live\n");

    serverImpl.ServerInit();
    server->Wait();
}

/*
*   @usage: ./server <my ip with port>  <lb ip with port>
*/
int main(int argc, char **argv) 
{
    // init
    g_stateHelper.SetIdentity(FOLLOWER);
    serverImpl.SetMyIp(argv[1]);

    pthread_t kv_server_t;
    pthread_t hb_t;
    
    pthread_create(&kv_server_t, NULL, RunKeyValueServer, argv[1]);
    pthread_create(&hb_t, NULL, StartHB, argv[2]);
    
    RunServer(argv[1]);

    pthread_join(hb_t, NULL);
    pthread_join(kv_server_t, NULL);

    return 0;
}