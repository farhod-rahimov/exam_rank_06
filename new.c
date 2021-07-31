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

void ft_add_new_send_msg(t_client *current, char *msg, int free_flag) {
    char *tmp;

    tmp = current->send_msg;
    if (tmp == NULL) {
        current->send_msg = (char *)malloc(sizeof(char) * strlen(msg) + 1);
        strcpy(current->send_msg, msg);
    }
    else {
        current->send_msg = (char *)malloc(sizeof(char) * (strlen(current->send_msg) + strlen(msg) + 1));
        size_t i = 0;
        for (; tmp[i]; i++){
            current->send_msg[i] = tmp[i];
        }
        for (size_t n = 0; msg[n]; n++) {
            current->send_msg[i++] = msg[n];
        }
        current->send_msg[i] = '\0';
    }
    current->remained_send = strlen(current->send_msg);
    if (tmp != NULL)
        free(tmp);
    if (free_flag)
        free(msg);
}

char * ft_edit_msg_for_send(int id, char *msg) {
    char *new;
    char *tmp;
    int num_lines;

    tmp = (char *)malloc(sizeof(char) * (strlen("client d: ") + 10)); // int can contain max 10 digits 
    sprintf(tmp, "client %d: ", id);
    
    num_lines = 1;
    for (size_t i = 0; msg[i]; i++) {
        if (msg[i] == '\n')
            num_lines++;
    }

    new = (char *)malloc(sizeof(char) * (strlen(msg) + (num_lines * strlen(tmp)) + 1));
    
    size_t i = 0;
    for (size_t n = 0; tmp[n]; n++) {
        new[i++] = tmp[n];
    }
    for (size_t k = 0; msg[k]; k++) {
        if (k > 0 && msg[k - 1] == '\n') {
            for (size_t n = 0; tmp[n]; n++) {
                new[i++] = tmp[n];
            }
            new[i++] = msg[k];
            continue;
        }
        new[i++] = msg[k];
    }
    new[i] = '\0';
    free(tmp);
    return(new);
}

void ft_copy_recv_buffer(t_client *current, char *buf) {
    char * tmp;
    
    tmp = current->recv_msg;
    current->recv_msg = (char *)malloc(sizeof(char) * strlen(buf) + 1);
    
    if (current->recv_msg == NULL) {
        printf("copy recv buffer ");
        write(2, "Fatal error\n", 12); exit(1);
    }
    
    strcpy(current->recv_msg, buf);
    
    if (tmp != NULL)
        free(tmp);
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
            current = tmp;
            sprintf(buf, "server: client %d just arrived\n", last_id - 1);
            for (; tmp != NULL; tmp = tmp->prev) {
                if (!tmp->removed && tmp->id != last_id - 1)
                    ft_add_new_send_msg(tmp, buf, 0);
            }
            tmp = current;
            continue;
        }
        for (tmp = head; tmp != NULL; ) {
            fd = tmp->fd;
            if (!tmp->removed) {
                if (FD_ISSET(fd, &readfds)) {
                    printf("%d READ_AVAILABLE\n", fd);
                    bzero(buf, buffer_size + 1);
                    ret = recv(fd, buf, buffer_size, 0);

                    if (ret == 0) {
                        tmp->removed = 1;
                        current = tmp;
                        sprintf(buf, "server: client %d just left\n", current->id);
                        for (tmp = head; tmp != NULL; tmp = tmp->next) {
                            if (!tmp->removed) {
                                ft_add_new_send_msg(tmp, buf, 0);
                            }
                        }
                        tmp = current;
                        tmp = tmp->next;
                        continue ;
                    }
                    else if (ret < 0) { // delete
                        printf("recv ");
                        write(2, "Fatal error\n", 12); exit(1);
                    }
                    buf[ret] = '\0';
                    printf("buf\n%s", buf);
                    ft_copy_recv_buffer(tmp, buf);
                    current = tmp;
                    for (tmp = head; tmp != NULL; tmp = tmp->next) {
                        if (tmp->fd != current->fd && !tmp->removed)
                            ft_add_new_send_msg(tmp, ft_edit_msg_for_send(current->id, current->recv_msg), 1);
                    }
                    tmp = current;
                }
                if (FD_ISSET(fd, &writefds)) {
                    if (!tmp->removed) {
                        printf("%d WRITE_AVAILABLE\n", fd);
                        already_sent = strlen(tmp->send_msg) - tmp->remained_send;
                        ret = send(tmp->fd, tmp->send_msg + already_sent, tmp->remained_send, 0);
                        if (ret != -1)
                            tmp->remained_send -= ret;
                        if (tmp->remained_send <= 0) {
                            free(tmp->send_msg);
                            tmp->send_msg = NULL;
                        }
                    }
                }
            }
            tmp = tmp->next;
        }
    }
}