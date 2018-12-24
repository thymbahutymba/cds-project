#include "manager.h"

struct PendingRequest *pr = NULL;
unsigned int r_available = N;
unsigned int resources[N];

// socket file descriptor for allocate and deallocate request
int sfd_all, sfd_deall;

void send_resource(int n_res, struct sockaddr_un *sender, int c_id) {
    int index;
    char r_name[BUFFER_SIZE];   // Name of the resource to send to the client
    char response[BUFFER_SIZE]; // Response message from the client
    int error;                  // Error received from client

    // Iterate over the number of resources requested
    for (; n_res; --n_res) {

        // Find the first resources available
        for (index = 0; index < N && resources[index]; index++)
            ;

        resources[index] = c_id;

        // Send resource name to client
        sprintf(r_name, R_NAME, index);
        port_send(sfd_all, r_name, strlen(r_name), NULL, &sender);

        // Receive response from client
        port_recv(sfd_all, response, BUFFER_SIZE, &sender);

        error = strtol(response, NULL, 10);
        if (error) {
            fprintf(stderr, "Error code: %d\n", error);
            exit(1);
        }
    }
}

/* Pools over allocation and deallocation requests */
int ex_poll() {
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

/* Create new request and return its pointer */
struct PendingRequest *create_request(struct sockaddr_un *sender,
                                      unsigned int r, unsigned int p,
                                      unsigned int c_id) {
    struct PendingRequest *nr;

    nr = (struct PendingRequest *)malloc(sizeof(struct PendingRequest));
    nr->c_sender = sender;
    nr->resources = r;
    nr->priority = p;
    nr->client_id = c_id;
    nr->next = NULL;

    return nr;
}

/* Inserts the request in the pending queue */
void insert_request(struct sockaddr_un *sender, unsigned int r,
                    unsigned int c_id) {
    struct PendingRequest *it;

    // there are no pending requests yet
    if (pr == NULL) {
        // creation of new pending request with lowest priority (0)
        pr = create_request(sender, r, 0, c_id);
        return;
    }

    // find last element in the queue
    for (it = pr; it->next != NULL; it = it->next)
        ;

    // Inserts request as last with the lowest priority (0)
    it->next = create_request(sender, r, 0, c_id);
}

/* Serves pending requests if there are enough resources available */
void serve_request() {
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

            if (it == pr) {
                pr = it->next;

                // Update the highest priority
                p_target = (pr != NULL) ? pr->priority : p_target;
            } else
                pred->next = it->next;

            // send positive reply of resources allocation to the client
            port_send(sfd_all, "0", 1, NULL, &(it->c_sender));

            send_resource(it->resources, it->c_sender, it->client_id);

            printf("Allocation of %i resources successful.\n", it->resources);

            // deallocation of memory previously allocated
            free(it->c_sender);
            free(it);
        }
    }

    // If a client has been served all queued clients have their priority
    // increased
    if (old_r != r_available)
        // update priority for each element in the queue
        for (it = pr; it != NULL; it = it->next)
            it->priority++;
}

void handle_allocation(unsigned int r, struct sockaddr_un *sender,
                       unsigned int c_id) {
    // available resources can handle request from client
    if (pr == NULL && r <= r_available) {
        r_available -= r;

        // send positive reply of resources allocation to client
        port_send(sfd_all, "0", 1, NULL, &sender);

        // Send resource id to client
        send_resource(r, sender, c_id);

        printf("Allocation of %i resources successful.\n", r);
    } else {
        // insert request to queue
        insert_request(sender, r, c_id);
    }
}

void handle_deallocation(unsigned int r, struct sockaddr_un *sender,
                         unsigned int c_id) {
    size_t index;

    // update resources available
    r_available += r;

    // Deallocation of resources previously allocated to the client
    for (index = 0; index < N; ++index) {
        resources[index] = (resources[index] == c_id) ? 0 : resources[index];
    }

    // send positive reply of resources deallocation
    port_send(sfd_deall, "0", 1, NULL, &sender);
    printf("Deallocation of %i resources successful.\n", r);

    serve_request();
    free(sender);
}

int main() {
    char buf[BUFFER_SIZE]; // buffer for receiving message
    char msg[BUFFER_SIZE]; // buffer for sending message
    unsigned int res;      // resources request received from client
    unsigned int c_id;     // client id that make request
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
        c_sfd = ex_poll();

        // recv message from given socket file descriptor
        memset(buf, 0, BUFFER_SIZE);
        port_recv(c_sfd, buf, BUFFER_SIZE, &c_sender);

        // get the client id and number of resources requested from it
        sscanf(buf, REQ_FORM, &c_id, &res);

        // Number of resources requested could exceed the max available from
        // manager
        if (res > N) {
            port_send(c_sfd, "-1", 2, NULL, &c_sender);
            free(c_sender);
            continue;
        }

        // Print debug message to stdout
        sprintf(msg, dbg[c_sfd == sfd_deall], res);
        printf("%s\n", msg);

        if (c_sfd == sfd_all)
            handle_allocation(res, c_sender, c_id);
        else {
            handle_deallocation(res, c_sender, c_id);
        }
    }

    return 0;
}