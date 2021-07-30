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
    int fd;
    int id;
    char *read_msg;
    char *send_msg;
    int remained_send;
    struct client *next;
    struct client *prev;
} t_client;


t_client *ft_create_new_client(t_client *prev, int fd, int id) {
    t_client *new;

    new = (t_client *)malloc(sizeof(t_client));
    if (new == NULL) {
        write(2, "6Fatal error\n", 13); exit(1);
    }
    if (prev != NULL)
        prev->next = new;
    new->prev = prev;

    new->fd =fd;
    new->id = id;
    new->read_msg = NULL;
    new->send_msg = NULL;
    new->remained_send = 0;
    new->next = NULL;
    return (new);
}

void ft_free_client(t_client *current) {
    if (current->read_msg != NULL)
        free(current->read_msg);
    current->read_msg = NULL;
    if (current->send_msg != NULL)
        free(current->send_msg);
    current->send_msg = NULL;
    close(current->fd);
    free(current);
    current = NULL;
}

void ft_add_new_send_msg(t_client *current, char *msg, int free_flag) {
    char *tmp;

    tmp = current->send_msg;
    if (current->send_msg == NULL) {
        current->send_msg = (char *)malloc(sizeof(char) * strlen(msg) + 1);
        strcpy(current->send_msg, msg);
        current->remained_send = strlen(current->send_msg);
        if (free_flag)
            free(msg);
        return ;
    }
    current->send_msg = (char *)malloc(sizeof(char) * (strlen(current->send_msg) + strlen(msg) + 1));
    size_t i = 0;
    for (; tmp[i]; i++){
        current->send_msg[i] = tmp[i];
    }
    for (size_t n = 0; msg[n]; n++) {
        current->send_msg[i++] = msg[n];
    }
    current->send_msg[i] = '\0';
    current->remained_send = strlen(current->send_msg);
    free(tmp);
    if (free_flag)
        free(msg);
}

char *ft_edit_msg_for_send(int id, char *msg) {
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
    free(tmp);
    return(new);
}

void ft_copy_recv_buffer(t_client *current, char *buf) {
    char * tmp;
    
    tmp = current->read_msg;
    if (tmp == NULL)
        current->read_msg = (char *)malloc(sizeof(char) * strlen(buf) + 1);
    else
        current->read_msg = (char *)malloc(sizeof(char) * strlen(buf) + strlen(tmp) + 1);
    if (current->read_msg == NULL) {
        write(2, "7Fatal error\n", 13); exit(1);
    }
    
    size_t i = 0;
    for (; tmp != NULL && tmp[i]; i++) {
        current->read_msg[i] = tmp[i];
    }
    for (size_t n = 0; buf[n]; n++) {
        current->read_msg[i++] = buf[n];
    }
    current->read_msg[i] = '\0';
    free(tmp);
}

int main(int argc, char ** argv) {
    int last_id = 0;
    int sock_fd, ret;
    struct sockaddr_in addr, client;
    socklen_t client_size;

    if (argc != 2) {
        write(2, "Wrong number of arguments\n", 26); exit(1);
    }

    sock_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        write(2, "1Fatal error\n", 13); exit(1);
    }


    bzero(&addr, sizeof(addr));
    addr.sin_family = PF_INET;
    addr.sin_port = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = htonl(2130706433);

    int opt = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); //delete

    ret = bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        write(2, "2Fatal error\n", 13); exit(1);
    }

    ret = listen(sock_fd, 15);
    if (ret < 0) {
        write(2, "3Fatal error\n", 13); exit(1);
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
        write(2, "8Fatal error\n", 13); exit(1);
    }

    head = NULL;
    current = head;
    while (1) {
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_SET(sock_fd, &readfds);

        for (tmp = head; tmp != NULL; tmp = tmp->next ) {
            FD_SET(tmp->fd, &readfds);
            if (tmp->remained_send > 0) {
                FD_SET(tmp->fd, &writefds);
            }
            if (tmp->fd > max_fd)
                max_fd = tmp->fd;
        }

        ret = select(max_fd + 1, &readfds, &writefds, NULL, NULL);
        if (ret < 1) {
            if (errno == EINTR) {
                write(2, "4Fatal error\n", 13); exit(1);
            }
            // continue;
        }
        if (FD_ISSET(sock_fd, &readfds)) {
            fd = accept(sock_fd, (struct sockaddr *)&client, &client_size);
            if (fd < 0) {
                write(2, "5Fatal error\n", 13); exit(1);
            }
            tmp = head;
            if (head == NULL) {
                head = ft_create_new_client(tmp, fd, last_id++);
            }
            else {
                for (tmp = head; tmp->next != NULL; tmp = tmp->next) {}
                tmp->next = ft_create_new_client(tmp, fd, last_id++);
            }
            current = tmp;
            for (; tmp != NULL; tmp = tmp->prev) {
                sprintf(buf, "server: client %d just arrived\n", last_id - 1);
                ft_add_new_send_msg(tmp, buf, 0);
            }
            tmp = current;
            continue;
        }
        for (tmp = head; tmp != NULL; ) {
            fd = tmp->fd;
            if (FD_ISSET(fd, &readfds)) {
                printf("%d READ_AVAILABLE\n", fd);
                bzero(buf, buffer_size + 1);
                ret = recv(fd, buf, buffer_size, 0);
                
                tmp_id = tmp->id;
                if (ret == 0) {
                    printf("%d CLOSED\n", fd);
                    if (tmp->next != NULL && tmp->prev == NULL) {
                        tmp->next->prev = tmp->prev;
                        current = tmp;
                        ft_free_client(current);
                    }
                    else if (tmp->next != NULL && tmp->prev != NULL) {
                        tmp->prev->next = tmp->next;
                        tmp->next->prev = tmp->prev;
                        current = tmp;
                        ft_free_client(current);
                    }
                    else if (tmp->next == NULL && tmp->prev == NULL) {
                        current = tmp;
                        ft_free_client(current);
                    }
                    else if (tmp->next == NULL && tmp->prev != NULL) {
                        tmp->prev->next = tmp->next;
                        current = tmp;
                        ft_free_client(tmp);
                    }
                    tmp = tmp->next;
                    current = tmp;
                    for (; tmp != NULL && tmp->prev != NULL; tmp = tmp->prev) {}
                    head = tmp;
                    sprintf(buf, "server: client %d just left\n", tmp_id);
                    int a = 0;
                    for (; tmp != NULL; tmp = tmp->next) {
                        ft_add_new_send_msg(tmp, buf, 0);
                        printf("%d\n",++a);
                    }
                    tmp = current;
                    continue ;
                }
                // else if (ret < 0) { // delete
                //     write(2, "9Fatal error\n", 13); exit(1);
                // }
                buf[ret] = '\0';
                ft_copy_recv_buffer(tmp, buf);
                current = tmp;
                for (tmp = head; tmp != NULL; tmp = tmp->next) {
                    if (tmp->fd != current->fd)
                        ft_add_new_send_msg(tmp, ft_edit_msg_for_send(current->id, current->read_msg), 1);
                }
                tmp = current;
            }
            if (FD_ISSET(fd, &writefds)) {
                already_sent = strlen(tmp->send_msg) - tmp->remained_send;
                ret = send(tmp->fd, tmp->send_msg + already_sent, tmp->remained_send, 0);
                tmp->remained_send -= ret;
                if (tmp->remained_send <= 0) {
                    free(tmp->send_msg);
                    tmp->send_msg = NULL;
                }
            }
            tmp = tmp->next;
        }
    }
    return (0);
}