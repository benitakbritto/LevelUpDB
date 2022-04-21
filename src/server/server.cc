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

/******************************************************************************
 * DECLARATION
 *****************************************************************************/

class KeyValueOpsServiceImpl final : public KeyValueOps::Service {

    public:
        Status GetFromDB(ServerContext* context, const GetRequest* request, GetReply* reply) override {
            dbgprintf("reached server\n");
            return Status::OK;
        }

        Status PutToDB(ServerContext* context,const PutRequest* request, PutReply* reply) override {
            dbgprintf("reached server\n");
            return Status::OK;
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

void ServerImplementation::SetMyIp(string ip)
{
    _myIp = ip;
}

string ServerImplementation::GetMyIp()
{
    return _myIp;
}


void ServerImplementation::ServerInit(const std::vector<string>& o_hostList) {    
    _stateHelper.SetIdentity(ServerIdentity::FOLLOWER);
    _stateHelper.AddCurrentTerm(0); // TODO @Shreyansh: need to read the term and not set to 0 at all times
    
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
    resetElectionTimeout();
    setAlarm(_electionTimeout);

    // TODO: Remove later
    if (_myIp == "0.0.0.0:50000") becomeLeader();
}

bool ServerImplementation::checkMajority() {
    int countSuccess = 0;
    for (auto& it: _appendEntriesResponseMap) {
        AppendEntriesReply replyReceived = it.second;
        if(replyReceived.success()) 
        {
            countSuccess++;
            if(2 * countSuccess == _hostList.size())
            {
                return true; // break on receiving majority
            }
        }
    }
    return false;
}

/* Candidate starts a new election */
void ServerImplementation::runForElection() {

    int initialTerm = _stateHelper.GetCurrentTerm();

    /* Vote for self - hence 1*/
    std::atomic<int> _votesGained(1);
    for (int i = 0; i < _hostList.size(); i++) {
        if (_hostList[i] != _myIp) {
        std::thread(&ServerImplementation::invokeRequestVote, this, _hostList[i], &_votesGained).detach();
        }
    }
    /* OPTIONAL - Can add sleep(_electionTimeout */
    sleep(1); //election timeout
    // while (_votesGained <= _hostCount/2 && nodeState == ServerIdentity::CANDIDATE &&
    //         currentTerm == initialTerm) {
    // }

    if (_votesGained > _hostCount/2 && _stateHelper.GetCurrentTerm() == ServerIdentity::CANDIDATE) {
        becomeLeader();
    }
}
        
void ServerImplementation::invokeRequestVote(string host, atomic<int> *_votesGained) {
    ClientContext context;
    context.set_deadline(chrono::system_clock::now() + 
        chrono::milliseconds(_heartbeatInterval));

    if(_stubs[host].get()==nullptr)
    {
        _stubs[host] = Raft::NewStub(grpc::CreateChannel(host, grpc::InsecureChannelCredentials()));
    }

    if(requestVote(_stubs[host].get()))
    {
        _votesGained++;
    }
    // TODO: Request Vote Code here
}

// TODO: Test 
// Leader makes this call to other nodes  
// TODO: param should also include log entries  
void ServerImplementation::invokeAppendEntries(string node_ip) 
{
    dbgprintf("[DEBUG] invokeAppendEntries: Entering function\n");
    // context.set_deadline(chrono::system_clock::now() + 
    //     chrono::milliseconds(_heartbeatInterval)); // QUESTION: Do we need this?
    
    ClientContext context;
    AppendEntriesRequest request;
    AppendEntriesReply reply;
    Status status;
    
    // TODO: Use state helper
    request.set_term(1); // TODO: Set appropriately
    request.set_leader_id("1"); // TODO: Set appropriately
    request.set_prev_log_index(1); // TODO: Set appropriately
    request.set_prev_log_term(1); // TODO: Set appropriately
    auto data = request.add_log_entry(); // TODO: Set appropriately
    data->set_log_index(1); // TODO: Set appropriately
    data->set_key("1"); // TODO: Set appropriately
    data->set_value("1"); // TODO: Set appropriately

    // TODO: Get stub from a global data structure
    auto stub = Raft::NewStub(grpc::CreateChannel(node_ip, grpc::InsecureChannelCredentials()));

    status = stub->AppendEntries(&context, request, &reply);
    dbgprintf("Status ok = %d\n", status.ok());
    _appendEntriesResponseMap[node_ip] = reply;
    dbgprintf("[DEBUG]: invokeAppendEntries: Exiting function\n");
}
        
bool ServerImplementation::requestVote(Raft::Stub* stub) {
    ReqVoteRequest req;
    req.set_term(_stateHelper.GetCurrentTerm());
    req.set_candidateid(_myIp);
    req.set_lastlogindex(_stateHelper.GetLogLength());
    req.set_lastlogterm(_stateHelper.GetTermAtIndex(_stateHelper.GetLogLength() - 1));

    ReqVoteReply reply;
    ClientContext context;

    Status status = stub->ReqVote(&context, req, &reply);

    setAlarm(_electionTimeout);

    return status.ok()? 1 : 0;
}

// TODO: Add params?
// Node calls this function after it becomes a leader  
void ServerImplementation::replicateEntries() 
{
    dbgprintf("[DEBUG] replicateEntries: Entering function\n");
    // setAlarm(_heartbeatInterval); // QUESTION: Is it needed?

    // TODO: Change this later
    vector<string> nodes_list;
    nodes_list.push_back("0.0.0.0:40000");
    nodes_list.push_back("0.0.0.0:40001");

    for (int i = 0; i < nodes_list.size(); i++) 
    {
        if (nodes_list[i] != _myIp) 
        {
            thread(&ServerImplementation::invokeAppendEntries, this, nodes_list[i]).detach();
        }
    }

    // QUESTION: Is there a case where the leader fails to reach majority???
    while(!checkMajority()){
        // keep waiting
    }
    dbgprintf("[DEBUG] replicateEntries: Exiting function\n");
}
        
void ServerImplementation::becomeLeader() {
    dbgprintf("[DEBUG] becomeLeader: Entering function\n");
    // TODO: become Leader Code here
    _stateHelper.SetIdentity(ServerIdentity::CANDIDATE); // TODO: Remove later
    _stateHelper.SetIdentity(ServerIdentity::LEADER);

    setNextIndexToLeaderLastIndex();
    // SetAlarm(heartbeatInterval); // QUESTION: Do we need this?
    replicateEntries();
    dbgprintf("[DEBUG] becomeLeader: Exiting function\n");
}

void ServerImplementation::setNextIndexToLeaderLastIndex() {
    // TODO: Fix this
    // int leaderLastIndex =  _stateHelper.GetNextIndex(_myIp);
    // _stateHelper.SetNextIndex(_myIp, leaderLastIndex);
}
        
void ServerImplementation::becomeFollower() {
    _stateHelper.SetIdentity(ServerIdentity::FOLLOWER);
}
        
/* 
    Invoked when timeout signal is received - 
    Increment current term and run for election

    TO-DO: Do not invoke if can_vote == false
*/

void ServerImplementation::becomeCandidate() {
    _stateHelper.SetIdentity(ServerIdentity::CANDIDATE);
    _stateHelper.AddCurrentTerm(_stateHelper.GetCurrentTerm() + 1);
    dbgprintf("INFO] Become Candidate\n");


    // TODO: Additional things here
    resetElectionTimeout();
    setAlarm(_electionTimeout);
    runForElection();
}

void ServerImplementation::resetElectionTimeout() {
    _electionTimeout = _minElectionTimeout + (rand() % 
        (_maxElectionTimeout - _minElectionTimeout + 1));
}

void ServerImplementation::setAlarm(int after_ms) {
    if (_stateHelper.GetIdentity() == ServerIdentity::FOLLOWER) {
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

Status ServerImplementation::AppendEntries(ServerContext* context, 
                                            const AppendEntriesRequest* request, 
                                            AppendEntriesReply *reply)
{
    dbgprintf("[DEBUG]: AppendEntries - Entering RPC\n");
    int my_term = 0;

    // TODO: Test Case 1
    // Case 1: leader term < my term
    my_term = _stateHelper.GetCurrentTerm();
    if (request->term() < my_term)
    {
        dbgprintf("[DEBUG]: AppendEntries RPC - In Case 1\n");
        reply->set_term(my_term);
        reply->set_success(false);
        return Status::OK;
    }
    // TODO: Test Case 1
    // Case 2: Candidate receives valid AppenEntries RPC
    else 
    {
        dbgprintf("[DEBUG]: AppendEntries RPC - In Case 2a\n");
        if (ServerIdentity::CANDIDATE)
        {
            becomeFollower();
        }

        // Check if term at log index matches
        if (_stateHelper.GetTermAtIndex(request->prev_log_index()) != request->prev_log_term())
        {
            reply->set_term(my_term);
            reply->set_success(false);
            return Status::OK;
        }
    }

    // TODO: Other cases

    dbgprintf("[DEBUG]: AppendEntries - Exiting RPC\n");
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


void RunServer(string my_ip, const std::vector<string>& hostList) {    
    /* TO-DO : Initialize GRPC connections to all other servers */

    ServerImplementation service;
    service.SetMyIp(my_ip);
    ServerBuilder builder;
    builder.SetMaxReceiveMessageSize((1.5 * 1024 * 1024 * 1024));
    builder.AddListeningPort(service.GetMyIp(), grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    unique_ptr<Server> server(builder.BuildAndStart());
	dbgprintf("[INFO] Server is listening on %s\n", service.GetMyIp().c_str());
    service.ServerInit(hostList);
    server->Wait();
}

int main(int argc, char **argv) {
    // TODO: Uncomment later
    // pthread_t kv_server_t;
    // pthread_t hb_t;
    
    // pthread_create(&kv_server_t, NULL, RunKeyValueServer, NULL);
    // pthread_create(&hb_t, NULL, StartHB, NULL);

    // pthread_join(hb_t, NULL);
    // pthread_join(kv_server_t, NULL);
    
    vector<string> hostList;
    RunServer(argv[1], hostList);
    return 0;
}