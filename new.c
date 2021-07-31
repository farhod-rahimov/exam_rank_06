#include <sys/socket.h>  // socket
#include <sys/types.h>   // connect
#include <netinet/in.h>  // struct sockaddr_in

#include <arpa/inet.h>   // inet_addr
#include <sys/select.h>  // select

#include <strings.h> // bzero
#include <stdlib.h> // atoi
#include <stdio.h> // sprintf

#include <unistd.h>
#include <errno.h>

typedef struct client {
    int     fd;
    int     id;
    int     removed;
    char    *recv_msg;
    char    *send_msg;
    int     remained_send;
    
    struct client *next;
    struct client *prev;
} t_client;

t_client *ft_create_new_client(t_client *prev, int fd, int id) {
    t_client *new;

    new = (t_client *)malloc(sizeof(t_client));
    if (new == NULL) {
        printf("create new client ");
        write(2, "Fatal error\n", 12); exit(1);
    }
    if (prev != NULL)
        prev->next = new;
    new->prev = prev;

    new->fd =fd;
    new->id = id;
    new->removed = 0;
    new->recv_msg = NULL;
    new->send_msg = NULL;
    new->remained_send = 0;
    new->next = NULL;
    return (new);
}

int main(int argc, char **argv) {
    int sock_fd, ret, last_id = 0;
    struct sockaddr_in addr, client;
    socklen_t client_size;

    if (argc != 2) {
        write(2, "Wrong number of arguments\n", 26); exit(1);
    }

    sock_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        write(2, "Wrong number of arguments\n", 26); exit(1);
    }

    bzero(&addr, sizeof(addr));
    addr.sin_family = PF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = htonl(2130706433);

    int opt = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); //delete

    ret = bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        printf("bind ");
        write(2, "Fatal error\n", 12); exit(1);
    }

    ret = listen(sock_fd, 15);
    if (ret < 0) {
        printf("listen ");
        write(2, "Fatal error\n", 12); exit(1);
    }

    bzero(&client, sizeof(client));
    client_size = sizeof(client);

    int fd, max_fd;
    fd_set readfds, writefds;
    max_fd = sock_fd;

    t_client *head;
    t_client *current;
    t_client *tmp;
    size_t buffer_size = 1000;
    int already_sent, tmp_id;
    char *buf = (char *)malloc(sizeof(char) * (buffer_size + 1));
    if (buf == NULL) {
        printf("buf malloc ");
        write(2, "Fatal error\n", 12); exit(1);
    }

    head = NULL;
    while (1) {
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_SET(sock_fd, &readfds);

        for (tmp = head; tmp != NULL; tmp = tmp->next ) {
            if (!tmp->removed) {
                FD_SET(tmp->fd, &readfds);
                if (tmp->remained_send > 0) {
                    FD_SET(tmp->fd, &writefds);
                }
                if (tmp->fd > max_fd)
                    max_fd = tmp->fd;
            }
        }
        ret = select(max_fd + 1, &readfds, &writefds, NULL, NULL);
        if (ret < 1) {
            if (errno == EINTR) {
                printf("select ");
                write(2, "Fatal error\n", 12); exit(1);
            }
            // continue;
        }
        if (FD_ISSET(sock_fd, &readfds)) {
            fd = accept(sock_fd, (struct sockaddr *)&client, &client_size);
            if (fd < 0) {
                printf("accept ");
                write(2, "Fatal error\n", 12); exit(1);
            }
            printf("ACCEPTED %d\n", fd);
            tmp = head;
            if (head == NULL) {
                head = ft_create_new_client(tmp, fd, last_id++);
            }
            else {
                for (; tmp->next != NULL; tmp = tmp->next) {}
                tmp->next = ft_create_new_client(tmp, fd, last_id++);
            }
            // current = tmp;
            // sprintf(buf, "server: client %d just arrived\n", last_id - 1);
            // for (; tmp != NULL; tmp = tmp->prev) {
            //     ft_add_new_send_msg(tmp, buf, 0);
            // }
            // tmp = current;
            // continue;
        }
        for (tmp = head; tmp != NULL; ) {
            fd = tmp->fd;
            if (!tmp->removed) {
                if (FD_ISSET(fd, &readfds)) {
                    printf("%d READ_AVAILABLE\n", fd);
                }
                if (FD_ISSET(fd, &writefds)) {
                    printf("%d WRITE_AVAILABLE\n", fd);
                }
            }
            tmp = tmp->next;
        }
    }
}