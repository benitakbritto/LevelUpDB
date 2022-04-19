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

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "server.h"
#include "lb.grpc.pb.h"

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
    int identity_enum = LEADER;

    LBNodeCommClient lBNodeCommClient(lb_addr, identity_enum, self_addr_lb);
    lBNodeCommClient.SendHeartBeat();

    return NULL;
}

void signalHandler(int signum) {
	return;
}

void ServerImplementation::serverInit(string ip, const std::vector<string>& o_hostList) {
    
    nodeState = ServerIdentity::FOLLOWER;
    currentTerm = 0;
    
    int old_errno = errno;
    errno = 0;
    signal(SIGALRM, &signalHandler);
    
    if (errno) {
        dbgprintf("ERROR] Signal could not be set\n");
        errno = old_errno;
        return;
    }
    errno = old_errno;
    srand(time(NULL));
    ResetElectionTimeout();
    SetAlarm(ServerImplementation::electionTimeout);
}
        
/* Candidate starts a new election */
void ServerImplementation::runForElection() {

    int initialTerm = currentTerm;

    /* Vote for self - hence 1*/
    std::atomic<int> votesGained(1);
    for (int i = 0; i < hostList.size(); i++) {
        if (hostList[i] != ip) {
        std::thread(&ServerImplementation::invokeRequestVote, this, hostList[i], &votesGained).detach();
        }
    }
    /* OPTIONAL - Can add sleep(electionTimeout */

    while (votesGained <= hostCount/2 && nodeState == ServerIdentity::CANDIDATE &&
            currentTerm == initialTerm) {
    }

    if (votesGained > hostCount/2 && nodeState == ServerIdentity::CANDIDATE) {
        BecomeLeader();
    }
}
        
void ServerImplementation::invokeRequestVote(string host, atomic<int> *votesGained) {
    ClientContext context;
    context.set_deadline(chrono::system_clock::now() + 
        chrono::milliseconds(heartbeatInterval));
    // TODO: Request Vote Code here
}
        
void ServerImplementation::invokeAppendEntries(int o_id) {
    ClientContext context;
    context.set_deadline(chrono::system_clock::now() + 
        chrono::milliseconds(heartbeatInterval));
    // TODO: Append Entries Code here
}
        
void ServerImplementation::requestVote() {
    // TODO: Request Vote Code here
    SetAlarm(electionTimeout);
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
    nodeState = ServerIdentity::LEADER;
    SetAlarm(heartbeatInterval);
}
        
void ServerImplementation::BecomeFollower() {
    nodeState = ServerIdentity::FOLLOWER;
}
        
/* 
    Invoked when timeout signal is received - 
    Increment current term and run for election

    TO-DO: Do not invoke if can_vote == false
*/

void ServerImplementation::BecomeCandidate() {
    nodeState = ServerIdentity::CANDIDATE;
    currentTerm++;
    dbgprintf("INFO] Become Candidate\n");


    // TODO: Additional things here
    ResetElectionTimeout();
    SetAlarm(electionTimeout);
    runForElection();
}

void ServerImplementation::ResetElectionTimeout() {
    electionTimeout = minElectionTimeout + (rand() % 
        (maxElectionTimeout - minElectionTimeout + 1));
}

void ServerImplementation::SetAlarm(int after_ms) {
    if (nodeState == ServerIdentity::FOLLOWER) {
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

Status ServerImplementation::AppendEntries(ServerContext* context, const AppendEntriesRequest* request, AppendEntriesReply *reply)
{
    return Status::OK;
}
Status ServerImplementation::ReqVote(ServerContext* context, const ReqVoteRequest* request, ReqVoteReply* reply)
{
    return Status::OK;
}
Status ServerImplementation::AssertLeadership(ServerContext* context, const AssertLeadershipRequest* request, AssertLeadershipReply* reply)
{
    return Status::OK;
}


void RunServer(const std::vector<string>& hostList) {
    string server_address("0.0.0.0:50051");

    /* TO-DO : Initialize GRPC connections to all other servers */

    ServerImplementation service;
    ServerBuilder builder;
    builder.SetMaxReceiveMessageSize((1.5 * 1024 * 1024 * 1024));
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    unique_ptr<Server> server(builder.BuildAndStart());
	dbgprintf("INFO] Server is live\n");

    service.serverInit(server_address, hostList);
    server->Wait();
}

int main(int argc, char **argv) {
    pthread_t hb_t;
    pthread_create(&hb_t, NULL, StartHB, NULL);
    pthread_join(hb_t, NULL);
    vector<string> hostList;
    RunServer(hostList);
    return 0;
}