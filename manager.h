#include "port.h"

#define N (10) // number of available resources
#define BUFFER_SIZE (80)

// List of pending request that can not be satisfied
struct PendingRequest {
    struct sockaddr_un *c_sender; // client that send request for resources
    unsigned int resources;       // resources required from client
    unsigned int priority;        // priority earned from client
    unsigned int client_id;
    struct PendingRequest *next;
};