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
    int id;
    int fd;
    int removed;
    int remained_send;
    char *send_msg;
    char *recv_msg;
    struct client *next;
} t_client;

t_client *ft_create_new_client(int fd, int id);
void ft_send_msg_to_all(t_client *head, t_client *current, char *msg, fd_set writefds);
void ft_edit_and_send_msg(t_client *head, t_client *current, char *buf, fd_set writefds);

int main(int argc, char **argv) {
    int fd, max_fd, last_id = 0, sock_fd, ret;
    fd_set readfds, writefds;

    struct sockaddr_in addr, client;
    socklen_t client_size = sizeof(client);

    if (argc != 2) {
        write(2, "Wrong number of arguments\n", 26); exit(1);
    }

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        printf("SOCKET\n");                             // delete!!!!!!!!!!
        write(2, "Fatal error\n", 12); exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = htonl(2130706433);

    ret = bind(sock_fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        printf("BIND\n");                             // delete!!!!!!!!!!
        write(2, "Fatal error\n", 12); exit(1);
    }

    ret = listen(sock_fd, 15);
    if (ret < 0) {
        printf("LISTEN\n");                             // delete!!!!!!!!!!
        write(2, "Fatal error\n", 12); exit(1);
    }

    int buffer_size = 3000;
    char *buf = (char *)malloc(sizeof(char) * (buffer_size + 1));
    if (buf == NULL) {
        printf("MALLOC\n");                             // delete!!!!!!!!!!
        write(2, "Fatal error\n", 12); exit(1);
    }

    t_client *head, *tmp, *current;
    head = NULL;
    while (1) {
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_SET(sock_fd, &readfds);
        max_fd = sock_fd;

        for (tmp = head; tmp != NULL; tmp = tmp->next) {
            if (!tmp->removed) {
                FD_SET(tmp->fd, &readfds);
                FD_SET(tmp->fd, &writefds);
                if (max_fd < tmp->fd) {
                    max_fd = tmp->fd;
                }
            }
        }

        ret = select(max_fd + 1, &readfds, &writefds, NULL, NULL);
        if (ret < 0) {                                              // ret < 0, pay attention
            continue ;
        }

        if (FD_ISSET(sock_fd, &readfds)) {
            fd = accept(sock_fd, (struct sockaddr *)&client, &client_size);
            if (fd < 0) {
                printf("ACCEPT\n");                             // delete!!!!!!!!!!
                write(2, "Fatal error\n", 12); exit(1);
            }
            if (head == NULL) {
                head = ft_create_new_client(fd, last_id++);
                tmp = head;
            }
            else {
                for (tmp = head; tmp->next != NULL; tmp = tmp->next) {}
                tmp->next = ft_create_new_client(fd, last_id++);
                tmp = tmp->next;
            }
            bzero(buf, buffer_size + 1);
            sprintf(buf, "server: client %d just arrived\n", tmp->id);
            ft_send_msg_to_all(head, tmp, buf, writefds);
            // continue;                                        // continue у столярова нет
        }
        for (tmp = head; tmp != NULL; tmp = tmp->next) {
            if (!tmp->removed) {
                if (FD_ISSET(tmp->fd, &readfds)) {
                    bzero(buf, buffer_size + 1);
                    ret = recv(tmp->fd, buf, buffer_size, 0);
                    if (ret <= 0) {
                        tmp->removed = 1;
                        close(tmp->fd);
                        bzero(buf, buffer_size + 1);
                        sprintf(buf, "server: client %d just left\n", tmp->id);
                        ft_send_msg_to_all(head, tmp, buf, writefds);
                    }
                    else {
                        buf[buffer_size] = '\0';
                        ft_edit_and_send_msg(head, tmp, buf, writefds);
                    }
                }
                // if (FD_ISSET(tmp->fd, &writefds)) {

                // }
            }
        }
    }
}

t_client *ft_create_new_client(int fd, int id) {
    t_client *new;

    new = (t_client *)malloc(sizeof(t_client));
    if (new == NULL) {
        printf("MALLOC\n");                             // delete!!!!!!!!!!
        write(2, "Fatal error\n", 12); exit(1);
    }

    new->id = id;
    new->fd = fd;
    new->removed = 0;
    new->remained_send = 0;
    new->send_msg = NULL;
    new->recv_msg = NULL;
    new->next = NULL;

    return (new);
}

void ft_send_msg_to_all(t_client *head, t_client *current, char *msg, fd_set writefds) {
    int ret;

    for (t_client *tmp = head; tmp != NULL; tmp = tmp->next) {
        if (!tmp->removed) {
            if (tmp->id != current->id && FD_ISSET(tmp->fd, &writefds)) {
                ret = send(tmp->fd, msg, strlen(msg), 0);
                if (ret < 0) {
                    printf("SEND\n");                             // delete!!!!!!!!!!
                    write(2, "Fatal error\n", 12); exit(1);
                }
            }
        }
    }
}

void ft_edit_and_send_msg(t_client *head, t_client *current, char *buf, fd_set writefds) {
    char *new_msg;
    char *prefix;
    int num_of_lines = 1;

    prefix = (char *)malloc(sizeof(char) * (strlen("client 2147483647: ") + 1));
    if (prefix == NULL) {
        printf("MALLOC\n");                             // delete!!!!!!!!!!
        write(2, "Fatal error\n", 12); exit(1);
    }
    sprintf(prefix, "client %d: ", current->id);

    for (size_t i = 0; buf[i]; i++) {
        if (buf[i] == '\n')
            num_of_lines++;
    }

    new_msg = (char *)malloc(sizeof(char) * (strlen(buf) + (strlen(prefix) * num_of_lines) + 1));
    if (new_msg == NULL) {
        printf("MALLOC\n");                             // delete!!!!!!!!!!
        write(2, "Fatal error\n", 12); exit(1);
    }

    size_t i = 0;
    for (; prefix[i]; i++) {
        new_msg[i] = prefix[i];
    }

    for (size_t k = 0; buf[k]; k++) {
        if (k > 0 && buf[k - 1] == '\n') {
            for (size_t l = 0; prefix[l]; l++) {
                new_msg[i++] = prefix[l];
            }
        }
        new_msg[i++] = buf[k];
    }
    new_msg[i] = '\0';
    ft_send_msg_to_all(head, current, new_msg, writefds);
    free(new_msg);
    free(prefix);
}