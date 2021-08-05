#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/select.h>


typedef struct s {
    int id;
    int fd;
    int removed;
    char *recv_msg;
    struct s *next;
} t_client;

t_client *ft_create_new_client(int fd, int id);
void ft_send_msg_to_all(t_client *head, t_client *current, char *msg, fd_set writefds);
void fd_edit_msg_and_send_to_all(t_client *head, t_client *current, char *msg, fd_set writefds);
void ft_copy_buf_to_client_struct(t_client *current, char *buf);

int main(int argc, char **argv) {
    int sock_fd, fd, max_fd, ret, last_id = 0;
    struct sockaddr_in addr, client;
    socklen_t client_size = sizeof(client);
    fd_set readfds, writefds;
    t_client *head, *tmp;
    size_t buffer_size = 3000;
    
    if (argc != 2) {
        write(2, "Wrong number of arguments\n", 26); exit(1);
    }

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        write(2, "Fatal error\n", 12); exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = htonl(2130706433);

    if (bind(sock_fd, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
        write(2, "Fatal error\n", 12); exit(1);
    }

    if (listen(sock_fd, 15) < 0) {
        write(2, "Fatal error\n", 12); exit(1);
    }

    head = NULL;
    char *buf = (char *)malloc(sizeof(char) * (buffer_size + 1));
    if (buf == NULL) {
        write(2, "Fatal error\n", 12); exit(1);
    }
    
    while (1) {
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_SET(sock_fd, &readfds);

        max_fd = sock_fd;
        for (tmp = head; tmp != NULL; tmp = tmp->next) {
            if (!tmp->removed) {
                FD_SET(tmp->fd, &readfds);
                FD_SET(tmp->fd, &writefds);
                if (max_fd < tmp->fd)
                    max_fd = tmp->fd;
            }
        }

        ret = select(max_fd + 1, &readfds, &writefds, NULL, NULL);
        if (ret < 0) {
            continue ;
        }

        if (FD_ISSET(sock_fd, &readfds)) {
            fd = accept(sock_fd, (struct sockaddr *)&client, &client_size);
            if (fd < 0) {
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
        }
        for (tmp = head; tmp != NULL; tmp = tmp->next) {
            if (!tmp->removed) {
                if (FD_ISSET(tmp->fd, &readfds)) {
                    ret = recv(tmp->fd, buf, buffer_size, 0);
                    if (ret <= 0) {
                        close(tmp->fd);
                        tmp->removed = 1;
                        bzero(buf, buffer_size + 1);
                        sprintf(buf, "server: client %d just left\n", tmp->id);
                        ft_send_msg_to_all(head, tmp, buf, writefds);
                    }
                    else {
                        buf[ret] = '\0';
                        if ((size_t)ret == buffer_size) {
                            ft_copy_buf_to_client_struct(tmp, buf);
                        }
                        else {
                            ft_copy_buf_to_client_struct(tmp, buf);
                            fd_edit_msg_and_send_to_all(head, tmp, tmp->recv_msg, writefds);
                            free(tmp->recv_msg);
                            tmp->recv_msg = NULL;
                        }
                    }
                }
            }
        }
    }
}

t_client *ft_create_new_client(int fd, int id) {
    t_client *new;

    new = (t_client *)malloc(sizeof(t_client));
    if (new == NULL) {
        write(2, "Fatal error\n", 12); exit(1);
    }

    new->id = id;
    new->fd = fd;
    new->removed = 0;
    new->recv_msg = NULL;
    new->next = NULL;
    return (new);
}

void ft_send_msg_to_all(t_client *head, t_client *current, char *msg, fd_set writefds) {
    for (t_client *tmp = head; tmp != NULL; tmp = tmp->next) {
        if (!tmp->removed && tmp->id != current->id && FD_ISSET(tmp->fd, &writefds)) {
            send(tmp->fd, msg, strlen(msg), 0);
        }
    }
}

void fd_edit_msg_and_send_to_all(t_client *head, t_client *current, char *msg, fd_set writefds) {
    char *new_msg;
    char *prefix;
    size_t num_lines = 1;

    for (size_t k = 0; msg[k]; k++) {
        if (msg[k] == '\n')
            num_lines++;
    }

    if ((prefix = (char *)malloc(sizeof(char) * (strlen("client 2147483647: ") + 1))) == NULL) {
        write(2, "Fatal error\n", 12); exit(1);
    }
    sprintf(prefix, "client %d: ", current->id);

    if ((new_msg = (char *)malloc(sizeof(char) * ((strlen(prefix) * num_lines) + strlen(msg) + 1))) == NULL) {
        write(2, "Fatal error\n", 12); exit(1);
    }

    size_t i = 0;
    for (size_t k = 0; prefix[k]; k++) {
        new_msg[i++] = prefix[k];
    }

    for (size_t j = 0; msg[j]; j++) {
        if (j > 0 && msg[j - 1] == '\n') {
            for (size_t k = 0; prefix[k]; k++) {
                new_msg[i++] = prefix[k];
            }
        }
        new_msg[i++] = msg[j];
    }
    new_msg[i] = '\0';
    ft_send_msg_to_all(head, current, new_msg, writefds);
    free(prefix);
    free(new_msg);
}

void ft_copy_buf_to_client_struct(t_client *current, char *buf) {
    char *tmp;
    size_t i;

    tmp = current->recv_msg;
    i = 0;
    if (current->recv_msg == NULL) {
        if ((current->recv_msg = (char *)malloc((sizeof(char) * (strlen(buf) + 1)))) == NULL) {
            write(2, "Fatal error\n", 12); exit(1);
        }
    }
    else {
        if ((current->recv_msg = (char *)malloc((sizeof(char) * (strlen(tmp) + strlen(buf) + 1)))) == NULL) {
            write(2, "Fatal error\n", 12); exit(1);
        }
        for (i = 0; tmp[i]; i++) {
            current->recv_msg[i] = tmp[i];
        }
    }
    for (size_t k = 0; buf[k]; k++) {
        current->recv_msg[i++] = buf[k];
    }
    current->recv_msg[i] = '\0';
}
