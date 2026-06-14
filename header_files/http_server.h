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
#include <iterator>
#include <unordered_map>
#include "request.h"
using std::string;

class Server {
private:
    const string port;
    int sockfd;
    int yes = 1;
    bool debug = true;
    
    void handle_client(int client_fd, char* ipstr);
    static string serve_template(string filename);
    static string serve_static(string filepath);
    static string parse_request(const Request& req);
    static string make_response(int status, string content_type, const string& body);
    static string get_content_type(string filepath);
    void send_msg(int client_fd, string msg, char* ipstr);
    void* get_in_addr(struct sockaddr *sa);
public:
    Server(string port);
    void start();
    void start_listening();
    void shutdown();
};