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
#include "file_handler.h"
#include "filesystem"
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
using namespace std::chrono_literals;
using std::string;

class Server {
private:
    const string port;
    string dir_name; // directory to serve
    
    int sockfd;
    int yes = 1;
    
    bool debug;
    std::atomic<bool> keep_running{true};
    
    std::chrono::steady_clock::time_point start_time;
    double duration{0};

    std::atomic<int> request_counter{0};
    inline static Server* instance = nullptr;
    int listener_id;
    
    void handle_client(int client_fd, char* ipstr);    
    Response router(const Request& req);
    Response handle_media_route(const Request& req);
    void send_msg(int client_fd, const string& msg, char* ipstr);
    void* get_in_addr(struct sockaddr *sa);
    bool check(int result, string& err_msg) const;
public:
    Server(string port, string dir_name, bool debug=false);
    
    void start();
    void listener(int thread_id);
    static void handle_signal(int signal_num); // handle ctrl-c signal
    void shutdown();
};