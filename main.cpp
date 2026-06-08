// simple stream server

#include <iostream>
#include <unistd.h>
#include <string>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cerrno>
#include <cstring>
#include <signal.h>
#include <sys/wait.h>

#define PORT "8080"
#define BACKLOG 10

void sigchld_handler(int s) {
    (void)s; // quiet unused variable warning

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

void* get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET)
        return &(((sockaddr_in*) sa)->sin_addr);
    else 
        return &(((sockaddr_in6*) sa)->sin6_addr);
}

int main(void) {
    int sockfd, new_fd;
    int yes = 1;

    addrinfo hints{}, *res; 
    hints.ai_family = AF_UNSPEC;    // either ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM;   // TCP
    hints.ai_flags = AI_PASSIVE;

    int status;
    if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << "\n";
        return 1;
    }

    // loop through the results linked list and bind to the first one that we can
    addrinfo* p;
    for (p = res; p != nullptr; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            std::cerr << "server: socket " << std::strerror(errno) << "\n";
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            std::cerr << "setsockopt: " << std::strerror(errno) << "\n";;
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            std::cerr << "server: bind " << std::strerror(errno) << "\n";;
            continue;
        }

        break;
    }
    freeaddrinfo(res);
    
    if (p == NULL) {
        std::cerr << "server failed to bind\n";
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        std::cerr << "listen: " << std::strerror(errno) << "\n";
        exit(1);
    }
    std::cout << "Listening on port: " << PORT << "\n";
    std::cout << "Waiting for connections...\n";
    
    // reap all dead children
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        std::cerr << "sigaction: " << std::strerror(errno) << "\n";
    }
    
    sockaddr_storage their_addr;
    socklen_t addr_size;
    while(true) {
        addr_size = sizeof (their_addr);
        int new_fd = accept(sockfd, (sockaddr*) &their_addr, &addr_size);
        if (new_fd == -1) {
            std::cerr << "accept: " << std::strerror(errno) << "\n";
            continue;
        }

        char ipstr[INET6_ADDRSTRLEN];
        inet_ntop(their_addr.ss_family, get_in_addr((sockaddr*) &their_addr), ipstr, sizeof(ipstr));
        std::cerr << "Connected to: " << ipstr << "\n";
        
        if (fork() == 0) { //child process
            close(sockfd); // child doesn't need the listener
         
            std::string msg = std::string("Hello, ") + ipstr + "\n";
            if (send(new_fd, msg.c_str(), msg.size(), 0) == -1) {
                std::cerr << "send: " << std::strerror(errno) << "\n";
            } else {
                std::cerr << "Message sent to client: " << ipstr << "\n";
            }
            
            close(new_fd);
            exit(0);
        }

        close(new_fd); // parent doesn't need this
    } 

    return 0;
}

