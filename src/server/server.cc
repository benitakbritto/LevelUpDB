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
#include <grpcpp/health_check_service_interface.h>

#include "raft.grpc.pb.h"
#include "lb.grpc.pb.h"
#include "../util/common.h"

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
int minElectionTimeout = 800;
int maxElectionTimeout = 1600;
int heartbeatInterval = 50;

class LBNodeCommClient {
  private:
    unique_ptr<LBNodeComm::Stub> stub_;
    Identity identity;
    string ip;
  
  public:
    LBNodeCommClient(string target_str, Identity _identity, string _ip) {
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
    Identity identity_enum = LEADER;

    LBNodeCommClient lBNodeCommClient(lb_addr, identity_enum, self_addr_lb);
    lBNodeCommClient.SendHeartBeat();

    return NULL;
}

void signalHandler(int signum) {
	return;
}

class ServerImplementation final : public Raft::Service {
    public:
        int nodeState = FOLLOWER;
        int currentTerm = 0;
        int electionTimeout;

        void serverInit() {
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
            SetAlarm(electionTimeout);
        }
        
        void switchToCandidate() {
            nodeState = CANDIDATE;
            currentTerm++;
			dbgprintf("INFO] Become Candidate\n");
            ResetElectionTimeout();
            SetAlarm(electionTimeout);
            runForElection();
        }

    
        void runForElection() {
			// TODO: Conduct Election
		}
        
		void invokeRequestVote(int voterId, atomic<int> *votesGained) {
            ClientContext context;
            context.set_deadline(chrono::system_clock::now() + 
                chrono::milliseconds(heartbeatInterval));
            // TODO: Request Vote Code here
        }
        
		void invokeAppendEntries(int o_id) {
            ClientContext context;
            context.set_deadline(chrono::system_clock::now() + 
                chrono::milliseconds(heartbeatInterval));
            // TODO: Append Entries Code here
        }
        
		void requestVote() {
            // TODO: Request Vote Code here
            SetAlarm(electionTimeout);
        }
        
		void appendEntries() {
            // TODO: Append Entries Code here 
            SetAlarm(electionTimeout);
        }
        
		void replicateEntries() {
            SetAlarm(heartbeatInterval);
            // TODO: Replicate to others
        }
        
		void BecomeLeader() {
            // TODO: Become Leader Code here
			nodeState = LEADER;
            SetAlarm(heartbeatInterval);
        }
        
		void BecomeFollower() {
            nodeState = FOLLOWER;
        }
        
		void BecomeCandidate() {
            nodeState = CANDIDATE;
            currentTerm++;
			dbgprintf("INFO] Become Candidate\n");

            // TODO: Additional things here
            ResetElectionTimeout();
            SetAlarm(electionTimeout);
            runForElection();
        }

        void ResetElectionTimeout() {
            electionTimeout = minElectionTimeout + (rand() % 
                (maxElectionTimeout - minElectionTimeout + 1));
        }
        
        void SetAlarm(int after_ms) {
            if (nodeState == FOLLOWER) {
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
};

void RunServer() {
    string server_address("0.0.0.0:50051");
    ServerImplementation service;
    ServerBuilder builder;
    builder.SetMaxReceiveMessageSize((1.5 * 1024 * 1024 * 1024));
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    unique_ptr<Server> server(builder.BuildAndStart());
	dbgprintf("INFO] Server is live\n");

    service.serverInit();
    server->Wait();
}

int main(int argc, char **argv) {
    pthread_t hb_t;
    pthread_create(&hb_t, NULL, StartHB, NULL);
    pthread_join(hb_t, NULL);
    RunServer();
    return 0;
}
