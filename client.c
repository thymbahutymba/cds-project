#include "port.h"

#define BUFFER_SIZE (80)

int main(int argc, char *argv[]) {
    int sfd_client;
    struct sockaddr_un *manager = NULL;
    char msg[BUFFER_SIZE];
    int error;

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
    port_send(sfd_client, argv[1], BUFFER_SIZE, ALL_R, &manager);

    // recv of answer for resources allocation
    port_recv(sfd_client, msg, BUFFER_SIZE, &manager);

    // Deallocation of memory allocated for comunication
    free(manager);
    manager = NULL;

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
    port_send(sfd_client, argv[1], BUFFER_SIZE, DEALL_R, &manager);

    // get reply of resources deallocation
    port_recv(sfd_client, msg, BUFFER_SIZE, &manager);
    if (!atoi(msg))
        printf("Resource deallocation successful\n");

    free(manager);
    return 0;
}
