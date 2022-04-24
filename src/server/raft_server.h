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
#include "../util/levelDBWrapper.h"
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
using kvstore::HeartBeatReply;

class RaftServer final : public Raft::Service {
private: 
  unordered_map<string, AppendEntriesReply> _appendEntriesResponseMap;
  MutexMap _lockHelper;
  string _myIp;

  LevelDBWrapper _levelDBWrapper;
  int _hostCount;
  atomic<int> _votesGained;
  int _electionTimeout;

  int _minElectionTimeout = 800;
  int _maxElectionTimeout = 1600;
  int _heartbeatInterval = 50;

  void setAlarm(int after_us);
  void resetElectionTimeout();

  void runForElection();
  void invokeRequestVote(string host);
  bool requestVote(Raft::Stub* stub);
  
  void invokeAppendEntries(string node_ip);
  void invokePeriodicAppendEntries();

  void becomeFollower();
  void becomeCandidate();
  void becomeLeader();

  void setNextIndexToLeaderLastIndex();
  void setMatchIndexToLeaderLastIndex();

  AppendEntriesRequest prepareRequestForAppendEntries(string followerip, int nextIndex);

  int GetMajorityCount();
 
public:
  void AlarmCallback();
  void SetMyIp(string ip);
  string GetMyIp();
  void Run();
  void Wait();
  void ServerInit();
  void ClearAppendEntriesMap();
  void BroadcastAppendEntries();
  bool ReceivedMajority();
  void ExecuteCommands(int start, int end);
  void BuildSystemStateFromHBReply(HeartBeatReply reply);

  grpc::Status AppendEntries(ServerContext* context, const AppendEntriesRequest* request, AppendEntriesReply *reply);
  grpc::Status ReqVote(ServerContext* context, const ReqVoteRequest* request, ReqVoteReply* reply);

};

#endif