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
using grpc::StatusCode;

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
            // cleanup apppendEntriesReply map
            // append() to log
            // broadcast AE 
            do {

            } while(!majority);
            
            // setCommitIdx(lastLogIdx)
            // for i in[lastApplied+1, commitIdx]
                // exec cmd at i
                // lastApplied ++

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

AppendEntriesRequest ServerImplementation::prepareRequestForAppendEntries (int nextIndex) {

    AppendEntriesRequest request;
    AppendEntriesReply reply;
    Status status;

    int retryCount = 0;
    int logLength = _stateHelper.GetLogLength();
    int prevLogIndex = _stateHelper.GetMatchIndex();
    int prevLogTerm = _stateHelper.GetTermAtIndex(prevLogIndex);

    request.set_term(_stateHelper.GetCurrentTerm()); 
    request.set_leader_id(_myIp); 
    request.set_prev_log_index(prevLogIndex); 
    request.set_prev_log_term(prevLogTerm);
    request.set_leader_commit_index(_stateHelper.GetCommitIndex());

    // TODO: Improve efficiency
    for(int i = nextIndex; i < logLength; i++) 
    {
        string key = _stateHelper.GetKeyAtIndex(i);
        string value = _stateHelper.GetValueAtIndex(i);

        auto data = request.add_log_entry();

        data->set_log_index(i); 
        data->set_key(key); 
        data->set_value(value);
    }

    return request;
}
// TODO: Test 
// Leader makes this call to other nodes  
// void ServerImplementation::invokeAppendEntries(string followerIp) 
// {
//     dbgprintf("[DEBUG] invokeAppendEntries: Entering function\n");
//     // context.set_deadline(chrono::system_clock::now() + 
//     //     chrono::milliseconds(_heartbeatInterval)); // QUESTION: Do we need this?
    
//     AppendEntriesRequest request;
//     AppendEntriesReply reply;
//     Status status;
//     int retryCount = 0;

//     int logLength = _stateHelper.GetLogLength();
//     int prevLogIndex = logLength-1; // TODO: Change on retry
//     int prevLogTerm = _stateHelper.GetTermAtIndex(prevLogIndex);
//     int nextIndex = _stateHelper.GetNextIndex();

//     // TODO: Get stub from a global data structure
//     auto stub = Raft::NewStub(grpc::CreateChannel(followerIp, grpc::InsecureChannelCredentials()));

//     // Retry w backoff
//     do
//     {
//         ClientContext context;
//         reply.Clear();
//         sleep(RETRY_TIME_START * retryCount * RETRY_TIME_MULTIPLIER);
        
//         status = stub->AppendEntries(&context, request, &reply);
        
//         retryCount++;
//     } while (status.error_code() == StatusCode::UNAVAILABLE);

//     dbgprintf("Status ok = %d\n", status.ok());
//     _appendEntriesResponseMap[followerIp] = reply;

//     // Check the reply of the RPC
//     if (request.term() < reply.term())
//     {
//         // Leader becomes follower
//         becomeFollower();
//         return;
//     }
//     // TODO: Retry RPC with different next index
//     else if (reply.success() == false)
//     {
 
//     }
//     // AppendEntries was successful for node
//     else if (reply.success() == true)
//     {
//         _stateHelper.SetMatchIndex(followerIp, logLength - 1);
//     }


//     dbgprintf("[DEBUG]: invokeAppendEntries: Exiting function\n");
// }
        
void ServerImplementation::invokeAppendEntries(string followerIp) {

    int nextIndex = _stateHelper.GetNextIndex();

    do 
    {
        AppendEntriesRequest request = prepareRequestForAppendEntries();
        auto stub = Raft::NewStub(grpc::CreateChannel(followerIp, grpc::InsecureChannelCredentials()));
        // call RPC with retry with backoff
      
        bool shouldRetry = (request.term() >= reply.term() || !reply.success())
        
        if (shouldRetry) {
            _stateHelper.SetNextIndex(nextIndex-1);
            _stateHelper.SetNextIndex(matchIndex-1);
        }

    } while (shouldRetry);

    if (request.term() < reply.term())
    {
        // Leader becomes follower
        becomeFollower();
    }
    else if(reply.success())
    {
        // setMatchIdx =  end of log
    }
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
void ServerImplementation::broadcastAppendEntries() 
{
    dbgprintf("[DEBUG] broadcastAppendEntries: Entering function\n");
    // setAlarm(_heartbeatInterval); // QUESTION: Is it needed?

    // TODO: Call dummyGetHostsList()
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
    
    // Stop this function if leader learns that it no longer is the leader
    if (_stateHelper.GetIdentity() == ServerIdentity::FOLLOWER)
    {
        return;
    }
    dbgprintf("[DEBUG] broadcastAppendEntries: Exiting function\n");
}
        
void ServerImplementation::becomeLeader() {
    dbgprintf("[DEBUG] becomeLeader: Entering function\n");
    // TODO: become Leader Code here
    _stateHelper.SetIdentity(ServerIdentity::CANDIDATE); // TODO: Remove later
    _stateHelper.SetIdentity(ServerIdentity::LEADER);

    setNextIndexToLeaderLastIndex();
    setMatchIndexToLeaderLastIndex();

    // TODO call AssertLeadership
    // create thread that calls invokePeriodicAppendEntries

    dbgprintf("[DEBUG] becomeLeader: Exiting function\n");
}

void ServerImplementation::invokePeriodicAppendEntries(){
    while (1)
    {
        // broadcast AE
        // sleep(heartbeat)
    }
}


void ServerImplementation::setNextIndexToLeaderLastIndex() {
    // TODO: Fix this for all hosts
    // int leaderLastIndex =  _stateHelper.GetNextIndex(_myIp);
    // _stateHelper.SetNextIndex(_myIp, leaderLastIndex);
}

void ServerImplementation::setMatchIndexToLeaderLastIndex() {
    // TODO: Fix this
    // int leaderLastIndex =  _stateHelper.GetNextIndex(_myIp);
    // _stateHelper.SetMatchIndex(_myIp, leaderLastIndex);
}

void ServerImplementation::dummySetHostList() {
    // TODO: Move node list here
}

void ServerImplementation::dummyGetHostList() {
    // TODO: Move node list here
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
        dbgprintf("[DEBUG]: AppendEntries RPC - Recever's term > request term\n");
        reply->set_term(my_term);
        reply->set_success(false);
        return Status::OK;
    }
    // TODO: Test Case 1
    // Case 2: Candidate receives valid AppendEntries RPC
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
            dbgprintf("[DEBUG]: AppendEntries RPC - term mismatch at log index\n");
            reply->set_term(my_term);
            reply->set_success(false);
            return Status::OK;
        } 
        else 
        {
            // TODO: Add to log from all entries, call Insert(prev_log_index+1, )
            // TODO: If LeaderCommitIdx > commitIdx 
                // commitIdx = leaderCommitIdx
                for(int i = _stateHelper.GetLastAppliedIndex(); i <= request->leader_commit_index(); i++)
                {
                    // exec all commands 
                    _stateHelper.SetLastAppliedIndex(i); 
                }
            reply->set_term(my_term);
            reply->set_success(true);
            return Status::OK;
        }
    }

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