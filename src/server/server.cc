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
#include "raft.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "server.h"
#include "lb.grpc.pb.h"
#include "../util/levelDBWrapper.h"
/******************************************************************************
 * NAMESPACES
 *****************************************************************************/
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::ServerReader;
using grpc::Status;
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

string self_addr_lb = "0.0.0.0:50051";
string lb_addr = "0.0.0.0:50056";

/******************************************************************************
 * DECLARATION
 *****************************************************************************/

class KeyValueOpsServiceImpl final : public KeyValueOps::Service {

    public:
        grpc::Status GetFromDB(ServerContext* context, const GetRequest* request, GetReply* reply) override {
            
            return grpc::Status::OK;
        }

        grpc::Status PutToDB(ServerContext* context,const PutRequest* request, PutReply* reply) override {
            dbgprintf("reached server\n");
            return grpc::Status::OK;
        }

};

void *RunKeyValueServer(void* args) {
    KeyValueOpsServiceImpl service;
    // string server_address(self_addr_lb);
    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    builder.AddListeningPort(self_addr_lb, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    
    cout << "[INFO] KeyValue Server listening on "<< self_addr_lb << std::endl;
    
    server->Wait();
    return NULL;
}

class LBNodeCommClient {
  private:
    unique_ptr<LBNodeComm::Stub> stub_;
    int identity;
    string ip;
  
  public:
    LBNodeCommClient(string target_str, int _identity, string _ip) {
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
            if(!stream->Write(request)) {
                break;
            }
            request.set_identity(identity);
            stream->Write(request);
            dbgprintf("INFO] SendHeartBeat: sent heartbeat\n");

            stream->Read(&reply);
            dbgprintf("[INFO] SendHeartBeat: recv heartbeat response\n");
          
            // TODO : Parse reply to get sys state

            dbgprintf("[INFO] SendHeartBeat: sleeping for 5 sec\n");
            sleep(HB_SLEEP_IN_SEC);
        }
        dbgprintf("[INFO] stream broke\n");
    }
};

void *StartHB(void* args) {
    int identity_enum = LEADER;

    LBNodeCommClient lBNodeCommClient(lb_addr, identity_enum, self_addr_lb);
    lBNodeCommClient.SendHeartBeat();

    return NULL;
}

ServerImplementation* signalHandlerService;
void signalHandler(int signum) {
    signalHandlerService->AlarmCallback();
	return;
}

void ServerImplementation::serverInit(string server_address) {
    
    hostList = {"0.0.0.0:50051", "0.0.0.0:50052", "0.0.0.0:50053"};
    ip = server_address;

    stateHelper.SetIdentity(ServerIdentity::FOLLOWER);

    //Initialize states
    stateHelper.AddCurrentTerm(0);
    stateHelper.SetCommitIndex(0);
    stateHelper.SetLastAppliedIndex(0);
    stateHelper.Append(0, "NULL", "NULL"); // To-Do: Need some initial data on file creation. Not a problem if file already exists.

    int old_errno = errno;
    errno = 0;
    cout<<"[INIT] Server Init"<<endl;
    signal(SIGALRM, &signalHandler);
    if (errno) {
        dbgprintf("[ERROR] Signal could not be set\n");
        errno = old_errno;
        return;
    }

    signalHandlerService = this;

    errno = old_errno;
    srand(time(NULL));
    ResetElectionTimeout();
    SetAlarm(electionTimeout);
}
        
/* Candidate starts a new election */
void ServerImplementation::runForElection() {   

    cout<<"[INFO]: Running for election"<<endl;
    int initialTerm = stateHelper.GetCurrentTerm();

    stateHelper.AddCurrentTerm(stateHelper.GetCurrentTerm() + 1);

    /* Vote for self - hence 1*/
    votesGained = 1;
    stateHelper.AddVotedFor(stateHelper.GetCurrentTerm(),ip);

    /*Reset Election Timer*/
    ResetElectionTimeout();
    SetAlarm(electionTimeout);

    /* Send RequestVote RPCs to all servers */
    for (int i = 0; i < hostList.size(); i++) {
        if (hostList[i] != ip) {
            std::thread(&ServerImplementation::invokeRequestVote, this, hostList[i]).detach();
        }
    }

    sleep(2);
    //cout<<"Woken up: " <<votesGained<<endl;
    if (votesGained > hostList.size()/2 && stateHelper.GetIdentity() == ServerIdentity::CANDIDATE) {
        cout<<"[INFO] Candidate received majority of "<<votesGained<<endl;
        cout<<"[INFO] Change Role to LEADER for term"<<stateHelper.GetCurrentTerm()<<endl;
        BecomeLeader();
    }
}
        
void ServerImplementation::invokeRequestVote(string host) {

    cout<<"[INFO]: Sending Request Vote to "<<host;
    
    if(stubs[host].get()==nullptr)
    {
        stubs[host] = Raft::NewStub(grpc::CreateChannel(host, grpc::InsecureChannelCredentials()));
    }

    if(requestVote(stubs[host].get()))
    {
        votesGained++;
    }
}
        
void ServerImplementation::invokeAppendEntries(int o_id) {
    ClientContext context;
    context.set_deadline(chrono::system_clock::now() + 
        chrono::milliseconds(heartbeatInterval));
    // TODO: Append Entries Code here
}
        
bool ServerImplementation::requestVote(Raft::Stub* stub) {
    ReqVoteRequest req;
    req.set_term(stateHelper.GetCurrentTerm());
    req.set_candidateid(ip);
    req.set_lastlogindex(stateHelper.GetLogLength());
    req.set_lastlogterm(stateHelper.GetTermAtIndex(stateHelper.GetLogLength()-1));
    ReqVoteReply reply;
    ClientContext context;
    context.set_deadline(chrono::system_clock::now() + 
        chrono::milliseconds(heartbeatInterval));

    grpc::Status status = stub->ReqVote(&context, req, &reply);

    SetAlarm(electionTimeout);

    if(status.ok() && reply.votegrantedfor())
        return true;
    
    return false;
}
        
void ServerImplementation::appendEntries() {
    // TODO: Append Entries Code here 
    SetAlarm(electionTimeout);
}
        
void ServerImplementation::replicateEntries() {
    SetAlarm(ServerImplementation::heartbeatInterval);
    // TODO: Replicate to others
}
        
void ServerImplementation::BecomeLeader() {
    // TODO: Become Leader Code here
    stateHelper.SetIdentity(ServerIdentity::LEADER);
    SetAlarm(heartbeatInterval);
    std::thread(&ServerImplementation::LeaderHB, this).detach();
}

void ServerImplementation::LeaderHB()
{
    //To-DO: Make it threaded not sequential.
    while(stateHelper.GetIdentity() == ServerIdentity::LEADER)
    {
        for(int i=0; i<hostList.size(); i++)
        {
            if(hostList[i]!=ip)
                ServerImplementation::invokeLeaderHB(hostList[i]);
                //std::thread(&ServerImplementation::invokeLeaderHB, this, hostList[i]).detach();
        }
        sleep(heartbeatInterval);
    }
}

void ServerImplementation::invokeLeaderHB(string host)
{
    if(stubs[host].get()==nullptr)
    {
        stubs[host] = Raft::NewStub(grpc::CreateChannel(host, grpc::InsecureChannelCredentials()));
    }
    if(host!=ip)
    {
        sendLeaderHB(stubs[host].get());
    }
}

void ServerImplementation::sendLeaderHB(Raft::Stub* stub)
{
    AssertLeadershipRequest request;
    AssertLeadershipReply reply;
    ClientContext context;

    request.set_leaderid(ip);

    stub->AssertLeadership(&context, request, &reply);

}
        
void ServerImplementation::BecomeFollower() {
    stateHelper.SetIdentity(ServerIdentity::FOLLOWER);
}
        
/* 
    Invoked when timeout signal is received - 
*/

void ServerImplementation::BecomeCandidate() {
    stateHelper.SetIdentity(ServerIdentity::CANDIDATE);
    dbgprintf("INFO] Become Candidate\n");
    ResetElectionTimeout();
    SetAlarm(electionTimeout);
    runForElection();
}

void ServerImplementation::AlarmCallback() {
  if (stateHelper.GetIdentity() == ServerIdentity::LEADER) {
    //ReplicateEntries();
  } else {
    BecomeCandidate();
  }
}

void ServerImplementation::ResetElectionTimeout() {
    electionTimeout = minElectionTimeout + (rand() % 
        (maxElectionTimeout - minElectionTimeout + 1));
}

void ServerImplementation::SetAlarm(int after_ms) {
    if (stateHelper.GetIdentity() == ServerIdentity::FOLLOWER) {
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

grpc::Status ServerImplementation::AppendEntries(ServerContext* context, const AppendEntriesRequest* request, AppendEntriesReply *reply)
{
    return grpc::Status::OK;
}

grpc::Status ServerImplementation::ReqVote(ServerContext* context, const ReqVoteRequest* request, ReqVoteReply* reply)
{
    cout<<"Received reqvote from "<<request->candidateid()<<" --- "<<request->term()<<endl;

    reply->set_votegrantedfor(false);
    reply->set_term(stateHelper.GetCurrentTerm());

    if(request->term() < stateHelper.GetCurrentTerm())
    {
        return grpc::Status::OK;
    }

    if(request->term() > stateHelper.GetCurrentTerm())
    {
        stateHelper.AddCurrentTerm(request->term());
        stateHelper.SetIdentity(ServerIdentity::FOLLOWER);
    }

    if(stateHelper.GetVotedFor(request->term())=="" || stateHelper.GetVotedFor(request->term()) == request->candidateid())
    {
        if(request->lastlogterm() > stateHelper.GetTermAtIndex(stateHelper.GetLogLength() - 1))
        {
            stateHelper.AddVotedFor(request->term(), request->candidateid());
            reply->set_votegrantedfor(true);

            return grpc::Status::OK;
        }

        if(request->lastlogterm() == stateHelper.GetTermAtIndex(stateHelper.GetLogLength() - 1))
        {
            if(request->lastlogindex() > stateHelper.GetLogLength()-1)
            {
                reply->set_votegrantedfor(true);

                return grpc::Status::OK;
            }
        }
    }

    return grpc::Status::CANCELLED;
}


/* Currently using AssertLeadership as heartbeat from leader to nodes. Will be using AppendEntries */
grpc::Status ServerImplementation::AssertLeadership(ServerContext* context, const AssertLeadershipRequest* request, AssertLeadershipReply* reply)
{
    cout<<"[INFO]: Received HB from Leader"<<endl;

    stateHelper.SetIdentity(ServerIdentity::FOLLOWER);
    ResetElectionTimeout();
    SetAlarm(electionTimeout);
    
    return grpc::Status::OK;
}


void RunServer(string server_address) {
    //string server_address("0.0.0.0:50051");

    /* TO-DO : Initialize GRPC connections to all other servers */

    ServerImplementation service;
    ServerBuilder builder;
    builder.SetMaxReceiveMessageSize((1.5 * 1024 * 1024 * 1024));
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    unique_ptr<Server> server(builder.BuildAndStart());
	  dbgprintf("[INFO] Server is live\n");

    service.serverInit(server_address);
    server->Wait();
}

int main(int argc, char **argv) {
    pthread_t kv_server_t;
    pthread_t hb_t;
    
    pthread_create(&kv_server_t, NULL, RunKeyValueServer, NULL);
    pthread_create(&hb_t, NULL, StartHB, NULL);
    string ip = argv[1];
    RunServer(ip);

    pthread_join(hb_t, NULL);
    pthread_join(kv_server_t, NULL);
    return 0;
}