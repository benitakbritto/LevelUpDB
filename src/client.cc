#include<errno.h>
#include "client.h"
#include <unistd.h>

Status KeyValueClient::GetFromDB(const GetRequest request, GetReply* reply) 
{
    int attempt = 0;
    Status status;

    do {
        reply->Clear();
        ClientContext context;
        status = stub_->GetFromDB(&context, request, reply);    
        attempt++;
    } while(attempt < 3 && status.error_code() != grpc::StatusCode::OK);

    return status;
}

Status KeyValueClient::PutToDB(const PutRequest request, PutReply* reply)
{
    dbgprintf("Reached client\n");
    
    Status status;
    ClientContext context;
    status = stub_->PutToDB(&context, request, reply);
    dbgprintf("[DEBUG] %s: status = %d\n", __func__, status.error_code());

    return status;
}
