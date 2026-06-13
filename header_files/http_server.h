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
#include <fstream>
#include "request.h"
using std::string;

class Server {
private:
    const string port;
    int sockfd;
    int yes = 1;
    
    void handle_client(int client_fd, char* ipstr);
    static string get_html(string filename);
    string parse_request(Request req);
    void send_msg(int client_fd, string msg, char* ipstr);
    void* get_in_addr(struct sockaddr *sa);
public:
    Server(string port);
    void start();
    void start_listening();
    void shutdown();
};