#include <sys/socket.h>  // socket
#include <sys/types.h>   // connect
#include <netinet/in.h>  // struct sockaddr_in
#include <iostream>

#include <arpa/inet.h>   // inet_addr
#include <sys/select.h>  // select
#include <fcntl.h>

#include <vector>
#include <unistd.h>


int main() {
    int socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    int ret;
    if (socket_fd < 0) std::cout << "SOCKET ERROR\n"; else std::cout << "SOCKET OK\n";

    struct sockaddr_in addr;
    addr.sin_family = PF_INET;
    addr.sin_port = htons(50001);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    ret = bind(socket_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) std::cout << "BIND ERROR\n"; else std::cout << "BIND OK\n";
    
    ret = listen(socket_fd, 50);
    if (ret < 0) std::cout << "LISTEN ERROR\n"; else std::cout << "LISTEN OK\n";
    int flags = fcntl(socket_fd, F_GETFL);
    std::cout << "FCNTL "  << fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) << std::endl;

    struct sockaddr_in client;
    socklen_t client_size = sizeof(client);
    
    const char * text = "HTTP/1.1 200 OK\nContent-Length: 19\nContent-Type: text/html\n\n<html>\nPOKA\n</html>";
    #define TEXT_LEN 79
    std::vector<int> client_fds;
    std::vector<int> responses;
    while (1) {
        int fd;
        fd_set readfds, writefds;
        int max_d = socket_fd;

        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_SET(socket_fd, &readfds);
        for (size_t i = 0; i < client_fds.size(); i++){
            fd = client_fds[i];
            FD_SET(fd, &readfds);
            if (responses[i] > 0)
                FD_SET(fd, &writefds);
            if (fd > max_d)
                max_d = fd;
        }

        int res = select(max_d + 1, &readfds, &writefds, NULL, NULL);
        if (res < 1) {
            if (errno == EINTR) {
                std::cout << "ERROR in SELECT\n";
            }
            else {
                std::cout << "SIGNAL CAME\n";
            }
            continue;
        }
        if (res == 0) {
            std::cout << "TIME OUT\n";
        }
        if (FD_ISSET(socket_fd, &readfds)) {
            int accept_fd = accept(socket_fd, (struct sockaddr *)&client, &client_size);
            if (accept_fd < 0) std::cout << "ACCEPT ERROR\n"; else std::cout << "ACCEPT OK\n";
            std::cout << "FCNTL "  << fcntl(accept_fd, F_SETFL, O_NONBLOCK) << std::endl;
            client_fds.push_back(accept_fd);
            responses.push_back(TEXT_LEN);
        }
        for (size_t i = 0; i < client_fds.size(); i++) {
            fd = client_fds[i];
            char buf[100];
            if (FD_ISSET(fd, &readfds) && (ret = recv(fd, &buf, 100, 0))) {
                buf[ret] = '\0';
                std::cout << "CLIENT SENT SOMETHING\n";
                std::cout << buf << std::endl;
            }
            if (FD_ISSET(fd, &writefds)) {
                ret = send(fd, text, TEXT_LEN, 0);
                if (ret < 0) std::cout << "SEND ERROR\n"; else  std::cout << "SEND OK\n";
                responses[i] -= ret;
                std::cout << "RET " << ret << std::endl;
            }
        }
    }

    return (0);
}