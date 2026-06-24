#pragma once

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
#include "request.h"
#include "response.h"
#include "filesystem"
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <queue>
#include <condition_variable>
#include <poll.h>
#include <fcntl.h>
#include "logger.h"
#include "http_connection.h"
#include "router.h"

using namespace std::chrono_literals;
using std::string;

struct Client {
    int fd;
    string ip;

    Client() = default;
    Client(int fd, char* ip) {
        this->fd = fd;
        this->ip = ip;
    }
};

class Server {
private:
    const string PORT;
    const string DIR_NAME; // directory to serve
    const unsigned int NUM_THREADS = 10;

    int sockfd;
    int yes = 1;
    int timeout = 5; // in seconds
    
    bool debug;
    std::atomic<bool> running{true};
    
    
    double duration{0};

    std::atomic<int> request_counter{0};
    inline static Server* instance = nullptr;

    std::queue<Client> client_q;
    std::mutex q_mutex;
    std::condition_variable cv;
    
    void handle_client(int client_fd, const string& client_ip);    
    Response router(const Request& req);
    Response handle_media_route(const Request& req);
    void* get_in_addr(struct sockaddr *sa);
public:
    Server(string port, string dir_name, bool debug=false);
    
    void start();
    void listener();
    void worker();
    static void handle_signal(int signal_num); // handle ctrl-c signal
    void shutdown();
};

