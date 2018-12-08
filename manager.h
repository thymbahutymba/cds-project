#include "port.h"

#define N (10) // number of available resources
#define BUFFER_SIZE (80)

// List of pending request that can not be satisfied
struct PendingRequest {
    struct sockaddr_un c_sender; // client that send request for resources
    unsigned int resources;      // resources required by client
    struct PendingRequest *next;
};