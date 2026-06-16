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
using std::string;

class Server {
private:
    const string port;
    int sockfd;
    int yes = 1;
    bool debug;
    string dir_name; // directory to serve
    
    void handle_client(int client_fd, char* ipstr);    
    Response router(const Request& req);
    Response handle_media_route(const Request& req);
    void send_msg(int client_fd, const string& msg, char* ipstr);
    void* get_in_addr(struct sockaddr *sa);
public:
    Server(string port, string dir_name, bool debug=false);
    void start();
    void start_listening();
    void shutdown();
};