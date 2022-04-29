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
void ResourceAllocator::AddServer()
{
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
        args[0] = "server";
        args[1] = "0.0.0.0:60000";
        args[2] = "0.0.0.0:600001";
        args[3] = "0.0.0.0:50052";
        args[4] = (char *) NULL;
        execv("./server", args);
        dbgprintf("[WARN] should not print\n");
    }
    else
    {
        // TODO: Remove
        dbgprintf("[INFO] parent\n");
    }
}


// TODO
void ResourceAllocator::DeleteServer()
{
    return;
}

/*
Test:
    cd src/cmake/build
    @usage  g++ -o r ../../server/resource_alloc_listener.cc -Wall && ./r
    @note   server's will be running in the backgroun
            ps -e | grep server
            kill -9 <pid>
*/
int main()
{
    ResourceAllocator obj;
    obj.AddServer();
    return 0;
}
