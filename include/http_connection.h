#pragma once

#include "request.h"
#include <iostream>
#include <string>
using std::string;
#include <sys/socket.h>
#include <fstream>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include "logger.h"
#include <optional>
#include "response.h"
#include "file_handler.h"
#include <chrono>

class HttpConnection {
public:
    int client_fd;
    const string& client_ip;
    string recv_buffer;
    const string& ROOT;
    std::chrono::nanoseconds recv_time{0};

    HttpConnection(int client_fd, const string& client_ip, const string& root);
    std::optional<Request> read_request(); 
    bool receieve(char* buffer, size_t buffer_size);
    void send_response(Response& res);
    void send_text(const string& msg);
    void send_file(Response& res);
    bool send_all(char* buffer, size_t chunk);
};

class MultipartParser {
public:
    MultipartParser(HttpConnection& conn);
    bool stream_to(const string& path);

    HttpConnection& conn;
    string filename;
    string boundary;
    bool read_subheaders();
    bool set_boundary(const Request& req);
};