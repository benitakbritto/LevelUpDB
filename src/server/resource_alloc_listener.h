#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include "../util/common.h"
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
#include <grpcpp/grpcpp.h>
#include "resalloc.grpc.pb.h"
#include "../util/common.h"

/******************************************************************************
 * NAMESPACES
 *****************************************************************************/
using namespace std;
using grpc::ClientContext;
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

/******************************************************************************
 * MACROS
 *****************************************************************************/

/******************************************************************************
 * GLOBALS
 *****************************************************************************/


/******************************************************************************
 * DECLARATION
 *****************************************************************************/
class ResourceAllocator final : public ResAlloc::Service
{
private:

public:
    grpc::Status AddServer(ServerContext* context, const AddServerRequest* request, AddServerReply* reply) override;
    grpc::Status DeleteServer(ServerContext* context, const DeleteServerRequest* request, DeleteServerReply* reply) override;
};