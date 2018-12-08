#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// port for sending/receiving the allocation requests of required resources
#define ALL_R ("/tmp/allocate_req")

// port for sending/receiving the deallocation of resources
#define DEALL_R ("/tmp/deallocate_req")

// client port name
#define C_PNAME ("/tmp/client_%d_r%u")

int port_open(const char *);
int port_send(int, const char *, size_t, const char *, struct sockaddr_un **);
int port_recv(int, const char *, size_t, struct sockaddr_un **);
