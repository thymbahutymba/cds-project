#include "port.h"

/*
 * Open local port with given name
 */
int port_open(const char *name) {
    int sock; // id of socket that will be created
    struct sockaddr_un port_addr;

    // create of local socket
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket");
        return -1;
    }

    if (strlen(name) > sizeof(port_addr.sun_path) - 1) {
        fprintf(
            stderr,
            "Socket name too long, the name can contain at most 107 chars.");
        return -1;
    }

    // remove file if already exists
    unlink(name);

    // Set type of socket and it's name.
    memset(&port_addr, 0, sizeof(struct sockaddr_un));
    port_addr.sun_family = AF_UNIX;
    strncpy(port_addr.sun_path, name, sizeof(port_addr.sun_path) - 1);

    if (bind(sock, (struct sockaddr *)&port_addr, sizeof(struct sockaddr_un)) <
        0) {
        perror("Bind");
        return -1;
    }

    return sock;
}

/*
 * Send message over specified port with given name
 */
int port_send(int sock, const char *msg, size_t size, const char *dst_name,
              struct sockaddr_un **dst) {
    socklen_t len = sizeof(struct sockaddr_un);
    ssize_t b_sent;

    // create dst if not exists
    if (*dst == NULL && dst_name != NULL) {
        *dst = (struct sockaddr_un *)malloc(sizeof(struct sockaddr_un));

        // set space taken by struct equal to 0
        memset(*dst, 0, sizeof(struct sockaddr_un));

        // create local port with given name
        (*dst)->sun_family = AF_UNIX;
        strncpy((*dst)->sun_path, dst_name, sizeof((*dst)->sun_path) - 1);
    }

    // send message over port
    b_sent = sendto(sock, msg, size, 0, (struct sockaddr *)*dst, len);
    if (b_sent < 0) {
        perror("SendTo");
    }

    return b_sent;
}

/*
 * Receive message from socket and store it into buffer
 */
int port_recv(int c_sfd, const char *buf, size_t size,
              struct sockaddr_un **sender) {
    ssize_t r_bytes; // bytes received from recvfrom
    socklen_t len = sizeof(struct sockaddr_un);

    r_bytes =
        recvfrom(c_sfd, (void *)buf, size, 0, (struct sockaddr *)*sender, &len);
    if (r_bytes < 0) {
        perror("RecvFrom");
        return -1;
    }

    return r_bytes;
}