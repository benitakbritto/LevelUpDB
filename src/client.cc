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

Status KeyValueClient::PutToDB(PutRequest request, PutReply* reply)
{
    Status status;
    int attempt = 0;
    int backoff = 2;
    dbgprintf("Reached client\n");
    do {
        sleep(attempt * backoff);
        // PutReply reply;
        ClientContext context;
        status = stub_->PutToDB(&context, request, reply);
        attempt++;
    } while(attempt < 3 && status.error_code() != grpc::StatusCode::OK);

    return status;
}
