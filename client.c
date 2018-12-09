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

    // creates string that contain an unique identifier and the resources
    // request
    sprintf(msg, C_PNAME, getpid(), atoi(argv[1]));

    // open port for current client
    sfd_client = port_open(msg);

    // send request for resources allocation to the manager
    b_sent = port_send(sfd_client, argv[1], BUFFER_SIZE, ALL_R, &manager);

    // recv of answer for resources allocation
    b_recv = port_recv(sfd_client, msg, BUFFER_SIZE, &manager);
    free(manager);
    manager = NULL;

    // if (sscanf(msg, "%i %i", &error, &n) && error == -1) {
    error = strtol(msg, NULL, 10);
    if (!error) {
        printf("Resource allocation successful\n");
    } else if (error == -1) {
        fprintf(stderr, "The resources request exceed the total resources "
                        "handled by the manager\n");
        return -1;
    }

    // use resources
    sleep(10);

    // deallocation of resources
    b_sent = port_send(sfd_client, argv[1], BUFFER_SIZE, DEALL_R, &manager);

    // get reply of resources deallocation
    b_sent = port_recv(sfd_client, msg, BUFFER_SIZE, &manager);
    if (!atoi(msg))
        printf("Resource deallocation successful\n");

    free(manager);
    return 0;
}
