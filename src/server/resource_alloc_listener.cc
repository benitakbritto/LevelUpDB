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
    dbgprintf("[DEBUG] %s| key = %s | pid = %d\n", __func__, key.c_str(), serverPid);
    serverMap[key] = serverPid;
}

// TODO: Error handling
int GetPidOfServer(string kvIp, string raftIp)
{
    string key = kvIp + ":" + raftIp;
    dbgprintf("[DEBUG] %s: key = %s\n", __func__, key.c_str());
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
        
        execv("./server", args);
        dbgprintf("[WARN] should not print\n");
    }
    // parent
    else
    {
        AddToMap(request->kv_ip(), request->raft_ip(), forkReturnCode);
    }

    dbgprintf("[DEBUG] %s: Exiting function\n", __func__);
    return grpc::Status::OK;
}

grpc::Status ResourceAllocator::DeleteServer(ServerContext* context, const DeleteServerRequest* request, DeleteServerReply* reply) 
{
    dbgprintf("[DEBUG] %s: Entering function\n", __func__);
    int pid = GetPidOfServer(request->kv_ip(), request->raft_ip());
    dbgprintf("[DEBUG] %s: pid = %d\n", __func__, pid);
    
    if (kill(pid, SIGKILL) == -1)
    {
        dbgprintf("[ERROR] %s: kill failed\n", __func__);
    }
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
    dbgprintf("resource alloc dead\n");
}

int main(int argc, char **argv) 
{
    RunResourceAllocatorServer();
    return 0;
}