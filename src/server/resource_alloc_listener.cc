#include "resource_alloc_listener.h"

/******************************************************************************
 * NAMESPACES
 *****************************************************************************/


/******************************************************************************
 * MACROS
 *****************************************************************************/


/******************************************************************************
 * GLOBALS
 *****************************************************************************/

/******************************************************************************
 * DECLARATION
 *****************************************************************************/
// TODO: Change to grpc server
// void ResourceAllocator::AddServer()
// {
//     int forkReturnCode = fork();
//     // fork failed
//     if (forkReturnCode < 0)
//     {
//         dbgprintf("[ERROR] fork failed\n");
//     }
//     // child process
//     else if (forkReturnCode == 0)
//     {
//         char* args[5];
//         args[0] = "server";
//         args[1] = "0.0.0.0:60000";
//         args[2] = "0.0.0.0:600001";
//         args[3] = "0.0.0.0:50052";
//         args[4] = (char *) NULL;
//         execv("./server", args);
//         dbgprintf("[WARN] should not print\n");
//     }
//     else
//     {
//         // TODO: Remove
//         dbgprintf("[INFO] parent\n");
//     }
// }


// TODO
// void ResourceAllocator::DeleteServer()
// {
//     return;
// }

// TODO
grpc::Status ResourceAllocator::AddServer(ServerContext* context, const AddServerRequest* request, AddServerReply* reply) 
{
    dbgprintf("[DEBUG] %s: Entering function\n", __func__);
    int forkReturnCode = fork();
    // fork failed
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
    else
    {
        // TODO: Remove
        dbgprintf("[INFO] parent\n");
    }
    dbgprintf("[DEBUG] %s: Exiting function\n", __func__);
    return grpc::Status::OK;
}

// TODO
grpc::Status ResourceAllocator::DeleteServer(ServerContext* context, const DeleteServerRequest* request, DeleteServerReply* reply) 
{
    dbgprintf("[DEBUG] %s: Entering function\n", __func__);
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

/*
Test:
    cd src/cmake/build
    @usage  g++ -o r ../../server/resource_alloc_listener.cc -Wall && ./r
    @note   server's will be running in the backgroun
            ps -e | grep server
            kill -9 <pid>
*/
// int main()
// {
//     ResourceAllocator obj;
//     obj.AddServer();
//     return 0;
// }

int main(int argc, char **argv) 
{
    RunResourceAllocatorServer();
    return 0;
}