#ifndef KEY_VALUE_SERVER_H
#define KEY_VALUE_SERVER_H

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

/******************************************************************************
 * DECLARATION
 *****************************************************************************/
class KeyValueOpsServiceImpl final : public KeyValueOps::Service 
{
public:
    Status GetFromDB(ServerContext* context, const GetRequest* request, GetReply* reply);
    Status PutToDB(ServerContext* context,const PutRequest* request, PutReply* reply);
};


#endif