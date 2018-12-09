#include "manager.h"

struct PendingRequest *pr = NULL;
unsigned int r_available = N;

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

// Create new request and return its pointer
struct PendingRequest *create_request(struct sockaddr_un *sender,
                                      unsigned int r, unsigned int p) {
    struct PendingRequest *nr;

    nr = (struct PendingRequest *)malloc(sizeof(struct PendingRequest));
    nr->c_sender = sender;
    nr->resources = r;
    nr->priority = p;
    nr->next = NULL;

    return nr;
}

void insert_request(struct sockaddr_un *sender, unsigned int r) {
    struct PendingRequest *it;

    // there are no pending requests
    if (pr == NULL) {
        // creation of new pending request with lowest priority (0)
        pr = create_request(sender, r, 0);
        return;
    }

    // find last element in the queue
    for (it = pr; it->next != NULL; it = it->next)
        ;

    if (!it->priority) {
        it->next = create_request(sender, r, 0);
    } else {
        it->next = create_request(sender, r, (it->priority - 1));
    }
}

void serve_request(int sfd) {
    struct PendingRequest *it, *pred;
    unsigned int p_target; // target priority of request that could be served
    const unsigned int old_r = r_available; // resources that were be available

    // there are no pending request in queue
    if (pr == NULL)
        return;

    // highest priority in the queue
    p_target = pr->priority;

    // find element at highest priority that can be served with available
    // resources
    for (it = pr; it != NULL && it->priority == p_target;
         pred = it, it = it->next) {

        // found element at highest priority that could be served
        if (it->resources <= r_available) {
            // update resources and queue
            r_available -= it->resources;

            if (it == pr)
                pr = it->next;
            else
                pred->next = it->next;

            // send positive reply of resources allocation to client
            port_send(sfd, "0", 1, NULL, &(it->c_sender));
            printf("Allocation of %i resources successful.\n", it->resources);

            // deallocation of allocated memory
            free(it->c_sender);
            free(it);
        }
    }

    if (old_r != r_available)
        // update priority for each element in the queue
        for (it = pr; it != NULL; it = it->next)
            it->priority++;
}

void handle_allocation(int sfd, unsigned int r, struct sockaddr_un *sender) {
    // available resources can handle request from client
    if (r <= r_available) {
        r_available -= r;

        // send positive reply of resources allocation to client
        port_send(sfd, "0", 1, NULL, &sender);
        printf("Allocation of %i resources successful.\n", r);
    } else {
        // insert request to queue
        insert_request(sender, r);
    }
}

void handle_deallocation(int sfd, unsigned int r, struct sockaddr_un *sender) {

    // update resources available
    r_available += r;

    // send positive reply of resources deallocation
    port_send(sfd, "0", 1, NULL, &sender);
    printf("Deallocation of %i resources successful.\n", r);

    serve_request(sfd);
    free(sender);
}

int main() {
    // socket file descriptor for allocate and deallocate request
    int sfd_all, sfd_deall;
    char buf[BUFFER_SIZE]; // buffer for receiving message
    char msg[BUFFER_SIZE]; // buffer for sending message
    unsigned int res;      // resources request received from client
    int c_sfd;
    char *dbg[] = {"Request for allocation %i resources.",
                   "Request for deallocation %i resources."};

    // open port for each type of request
    sfd_all = port_open(ALL_R);
    sfd_deall = port_open(DEALL_R);
    if (sfd_all < 0 || sfd_deall < 0) {
        return -1;
    }

    while (1) {
        // client that send the message
        struct sockaddr_un *c_sender =
            (struct sockaddr_un *)malloc(sizeof(struct sockaddr_un));

        // poll over guards and get file descriptor of incoming requests
        c_sfd = ex_poll(sfd_all, sfd_deall);

        // recv message from given socket file descriptor
        memset(buf, 0, BUFFER_SIZE);
        port_recv(c_sfd, buf, BUFFER_SIZE, &c_sender);

        // get number of resources requested by client
        res = atoi(buf);

        // the number of resources requested could exceed the max available from
        // manager
        if (res > N) {
            port_send(c_sfd, "-1", 2, NULL, &c_sender);
            free(c_sender);
            continue;
        }

        sprintf(msg, dbg[c_sfd == sfd_deall], res);
        printf("%s\n", msg);

        if (c_sfd == sfd_all)
            handle_allocation(c_sfd, res, c_sender);
        else {
            handle_deallocation(c_sfd, res, c_sender);
        }
    }

    return 0;
}