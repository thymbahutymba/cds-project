#include "port.h"

#define BUFFER_SIZE (80)

int main(int argc, char *argv[]) {
    int sfd_client;
    struct sockaddr_un *manager = NULL;
    ssize_t b_sent;
    ssize_t b_recv;
    char msg[BUFFER_SIZE];
    int error, n;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <resources>\n", argv[0]);
        return -1;
    }
    sprintf(msg, C_PNAME, getpid(), atoi(argv[1]));

    // open port for current client
    sfd_client = port_open(C_PNAME);

    // send request for resources allocation
    b_sent = port_send(sfd_client, msg, BUFFER_SIZE, ALL_R, &manager);

    // recv of answer for resources allocation
    b_recv = port_recv(sfd_client, msg, BUFFER_SIZE, &manager);

    if (sscanf(msg, "%i %i", &error, &n) && error == -1) {
        fprintf(stderr, "The resources obtainable from manager are at most %i.\n", n);
        return -1;
    }

    // use resources

    // deallocation of resources

    printf("%i\n", b_sent);
    free(manager);
}
