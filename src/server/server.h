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
// #include "../util/levelDBWrapper.h"
#include <csignal>
#include <ctime>
#include <cerrno>

using namespace std;
using grpc::Status;
using grpc::Server;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using blockstorage::Raft;
using blockstorage::AppendEntriesRequest;
using blockstorage::AppendEntriesReply;
using blockstorage::ReqVoteRequest;
using blockstorage::ReqVoteReply;
using blockstorage::AssertLeadershipRequest;
using blockstorage::AssertLeadershipReply;
using blockstorage::HeartBeatReply;

class ServerImplementation final : public Raft::Service {
private: 
  unordered_map<string, AppendEntriesReply> _appendEntriesResponseMap;
  MutexMap _lockHelper;
  string _myIp;
  std::map<string,std::unique_ptr<Raft::Stub>> _stubs; // TODO: use nodes
  const std::vector<std::string> _hostList; // TODO: use nodes

  // LevelDBWrapper _levelDBWrapper;
  int _hostCount;
  int _votesGained;
  int _electionTimeout;

  int _minElectionTimeout = 800;
  int _maxElectionTimeout = 1600;
  int _heartbeatInterval = 50;

  void setAlarm(int after_us);
  void resetElectionTimeout();

  void runForElection();
  void invokeRequestVote(string host, std::atomic<int> *votesGained);
  bool requestVote(Raft::Stub* stub);

  void invokeAppendEntries(string node_ip);
  void invokePeriodicAppendEntries();

  void becomeFollower();
  void becomeCandidate();
  void becomeLeader();

  void setNextIndexToLeaderLastIndex();
  void setMatchIndexToLeaderLastIndex();

  vector<string> dummyGetHostList(); // TODO: Replace with getHostList
  void dummySetHostList();

  AppendEntriesRequest prepareRequestForAppendEntries(string followerip, int nextIndex);

  int GetMajorityCount();


public:
  void SetMyIp(string ip);
  string GetMyIp();
  void Run();
  void Wait();
  void ServerInit(const std::vector<string>& o_hostList);
  void ClearAppendEntriesMap();
  void BroadcastAppendEntries();
  bool ReceivedMajority();
  void ExecuteCommands(int start, int end);
  void BuildSystemStateFromHBReply(HeartBeatReply reply);

  Status AppendEntries(ServerContext* context, const AppendEntriesRequest* request, AppendEntriesReply *reply) override;
  Status ReqVote(ServerContext* context, const ReqVoteRequest* request, ReqVoteReply* reply) override;
  Status AssertLeadership(ServerContext* context, const AssertLeadershipRequest* request, AssertLeadershipReply* reply) override;
};

#endif