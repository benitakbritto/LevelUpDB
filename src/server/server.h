#ifndef SERVER_H
#define SERVER_H

#include <memory>
#include <vector>
#include <mutex>
#include <random>
#include <list>

#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>
#include "raft.grpc.pb.h"
#include "../util/locks.h"
#include "../util/common.h"
#include "../util/state_helper.h"

#include <csignal>
#include <ctime>
#include <cerrno>

using namespace std;
using grpc::Status;
using grpc::Server;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using kvstore::Raft;
using kvstore::AppendEntriesRequest;
using kvstore::AppendEntriesReply;
using kvstore::ReqVoteRequest;
using kvstore::ReqVoteReply;
using kvstore::AssertLeadershipRequest;
using kvstore::AssertLeadershipReply;

class ServerImplementation final : public Raft::Service {
 public:
  void Run();
  void Wait();
  void serverInit(string ip, const std::vector<string>& o_hostList);
  MutexMap lockHelper;
  string ip;
  StateHelper stateHelper;
  std::map<string,std::unique_ptr<Raft::Stub>> stubs;
  const std::vector<std::string> hostList;

  Status AppendEntries(ServerContext* context, const AppendEntriesRequest* request, AppendEntriesReply *reply) override;
  Status ReqVote(ServerContext* context, const ReqVoteRequest* request, ReqVoteReply* reply) override;
  Status AssertLeadership(ServerContext* context, const AssertLeadershipRequest* request, AssertLeadershipReply* reply) override;

  void runForElection();
  void replicateEntries();
  void invokeRequestVote(string host, std::atomic<int> *votesGained);
  void invokeAppendEntries(int o_id);
  bool requestVote(Raft::Stub* stub);
  void appendEntries();


  void BecomeFollower();
  void BecomeCandidate();
  void BecomeLeader();

  void SetAlarm(int after_us);
  void ResetElectionTimeout();

  int hostCount;
  int votesGained;
  int electionTimeout;

  int minElectionTimeout = 800;
  int maxElectionTimeout = 1600;
  int heartbeatInterval = 50;
};

#endif