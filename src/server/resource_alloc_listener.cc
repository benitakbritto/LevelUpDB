#include "resource_alloc_listener.h"
#include <unordered_map>

/******************************************************************************
 * NAMESPACES
 *****************************************************************************/


/******************************************************************************
 * MACROS
 *****************************************************************************/


/******************************************************************************
 * GLOBALS
 *****************************************************************************/
unordered_map<string, int> serverMap; // kvIp:raftIp, pid


// TODO: Error handling
void AddToMap(string kvIp, string raftIp, int serverPid)
{
    string key = kvIp + ":" + raftIp;
    serverMap[key] = serverPid;
}

// TODO: Error handling
int GetPidOfServer(string kvIp, string raftIp)
{
    string key = kvIp + ":" + raftIp;
    return serverMap[key];
}


/******************************************************************************
 * DECLARATION
 *****************************************************************************/

grpc::Status ResourceAllocator::AddServer(ServerContext* context, const AddServerRequest* request, AddServerReply* reply) 
{
    dbgprintf("[DEBUG] %s: Entering function\n", __func__);
    int forkReturnCode = fork();
    
    if (forkReturnCode < 0)
    {
        dbgprintf("[ERROR] fork failed\n");
    }
    // child process
    else if (forkReturnCode == 0)
    {
        char* args[5];
        args[0] = (char *) string("server").c_str();
        args[1] = (char *) request->kv_ip().c_str();
        args[2] = (char *) request->raft_ip().c_str();
        args[3] = (char *) request->lb_ip().c_str();
        args[4] = (char *) NULL;
        
        dbgprintf("[DEBUG] %s: Child pid = %d\n", __func__, getpid());
        AddToMap(request->kv_ip(), request->raft_ip(), getpid());
        
        execv("./server", args);
        dbgprintf("[WARN] should not print\n");
    }

    dbgprintf("[DEBUG] %s: Exiting function\n", __func__);
    return grpc::Status::OK;
}

// TODO Test
grpc::Status ResourceAllocator::DeleteServer(ServerContext* context, const DeleteServerRequest* request, DeleteServerReply* reply) 
{
    dbgprintf("[DEBUG] %s: Entering function\n", __func__);
    int pid = GetPidOfServer(request->kv_ip(), request->raft_ip());
    dbgprintf("[DEBUG] %s: pid = %d\n", __func__, pid);
    string command = "kill -9 " + to_string(pid);
    system(command.c_str());
    dbgprintf("[DEBUG] %s: Exiting function\n", __func__);
    return grpc::Status::OK;
}

void RunResourceAllocatorServer() 
{
    ResourceAllocator service;
    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    builder.AddListeningPort(RES_ALLOC_SERVER_IP, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    unique_ptr<Server> server(builder.BuildAndStart());
    
    cout << "[INFO] Resource Allocator Server listening on "<< RES_ALLOC_SERVER_IP << endl;
    
    server->Wait();
}

int main(int argc, char **argv) 
{
    RunResourceAllocatorServer();
    return 0;
}