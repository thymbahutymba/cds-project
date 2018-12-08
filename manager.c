#include "manager.h"

struct PendingRequest pr = NULL;
unsigned int r_available;

int ex_poll(int sfd_all, int sfd_deall) {
    int c_sfd; // socker file descriptor for request received by client
    ssize_t res;
    struct pollfd guards[2]; // file descriptor to be monitored from poll

    guards[0].fd = sfd_all;
    guards[0].events = POLLIN;
    guards[1].fd = sfd_deall;
    guards[1].events = POLLIN;

    res = poll(guards, 2, -1);

    if (res <= 0) {
        fprintf(stderr, "Error: poll() returned %ld\n", res);
    }

    if (guards[0].revents == POLLIN) {
        c_sfd = sfd_all;
    } else if (guards[1].revents == POLLIN) {
        c_sfd = sfd_deall;
    } else {
        fprintf(stderr, "Error: poll() returned strange revents!!!\n");
    }

    return c_sfd;
}

void handle_allocation(struct sockaddr_un *sender) {

}

void handle_deallocation(struct sockaddr_un *sender) {

}

int main() {
    // socket file descriptor for allocate and deallocate request
    int sfd_all, sfd_deall;
    char buf[BUFFER_SIZE];
    char error[BUFFER_SIZE];
    unsigned int res; // resources request received from client
    int c_id;         // id of client that made the request

    // open port for each type of request
    sfd_all = port_open(ALL_R);
    sfd_deall = port_open(DEALL_R);
    if (sfd_all < 0 || sfd_deall < 0) {
        return -1;
    }

    while (1) {
        int c_sfd;

        // client that send the message
        struct sockaddr_un *c_sender =
            (struct sockaddr_un *)malloc(sizeof(struct sockaddr_un));

        // poll over guards and get file descriptor of incoming requests
        c_sfd = ex_poll(sfd_all, sfd_deall);

        // recv message from given socket file descriptor
        memset(buf, 0, BUFFER_SIZE);
        port_recv(c_sfd, buf, BUFFER_SIZE, &c_sender);

        // get number of resources requested by client
        sscanf(buf, C_PNAME, &c_id, &res);

        // the number of resources requested could exceed the max available from
        // manager
        if (res > N) {
            sprintf(error, "-1 %i", N);
            port_send(c_sfd, error, BUFFER_SIZE, NULL, &c_sender);
            free(c_sender);
            continue;
        }

        if (c_sfd == sfd_all)
            handle_request(c_sender);
        else
            handle_deallocation(c_sender);
    }

    return 0;
}