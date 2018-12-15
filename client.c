#include "port.h"

#define BUFFER_SIZE (80)

struct res_list {
    char name[BUFFER_SIZE];
    struct res_list *next;
};

struct res_list *r_recv = NULL;

void get_resources(struct sockaddr_un *manager, int sfd, size_t res) {
    printf("Resources received:\n");

    for (; res; --res) {
        struct res_list *it;
        struct res_list *targ;

        if (r_recv == NULL) {
            r_recv = malloc(sizeof(struct res_list));
            r_recv->next = NULL;
            targ = r_recv;
        } else {
            for (it = r_recv; it != NULL; targ = it, it = it->next)
                ;
            targ->next = malloc(sizeof(struct res_list));
            targ = targ->next;
            targ->next = NULL;
        }

        // Get resources from manager and send positive reply
        port_recv(sfd, targ->name, BUFFER_SIZE, &manager);
        port_send(sfd, "0", 1, NULL, &manager);

        printf("- %s\n", targ->name);
    }
}

int main(int argc, char *argv[]) {
    int sfd_client;
    struct sockaddr_un *manager = NULL;
    char msg[BUFFER_SIZE];
    char req_f[BUFFER_SIZE];
    int error;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <resources>\n", argv[0]);
        return -1;
    }

    printf("Request of %s resources\n", argv[1]);

    // creates string that contain an unique identifier and the resources
    // request
    sprintf(msg, C_PNAME, getpid());

    // open port for current client
    sfd_client = port_open(msg);

    // make request for sending to manager
    sprintf(req_f, REQ_FORM, getpid(), atoi(argv[1]));

    // send request for resources allocation to the manager
    port_send(sfd_client, req_f, strlen(req_f), ALL_R, &manager);

    // recv of answer for resources allocation
    port_recv(sfd_client, msg, BUFFER_SIZE, &manager);

    error = strtol(msg, NULL, 10);
    if (!error) {
        printf("Resource allocation successful\n");
    } else if (error == -1) {
        fprintf(stderr, "The resources request exceed the total resources "
                        "handled by the manager\n");
        return -1;
    }

    get_resources(manager, sfd_client, (size_t)atoi(argv[1]));

    // Deallocation of memory allocated for comunication
    free(manager);
    manager = NULL;

    // use resources
    sleep(10);

    // deallocation of resources
    port_send(sfd_client, req_f, strlen(req_f), DEALL_R, &manager);

    // get reply of resources deallocation
    port_recv(sfd_client, msg, BUFFER_SIZE, &manager);
    if (!atoi(msg))
        printf("Resource deallocation successful\n");

    while (r_recv != NULL) {
        struct res_list *tmp = r_recv;
        r_recv = r_recv->next;
        free(tmp);
    }

    free(manager);
    return 0;
}
