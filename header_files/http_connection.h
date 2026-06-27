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

class HttpConnection {
private:
    int client_fd;
    const string& client_ip;
public:
    HttpConnection(int client_fd, const string& client_ip);
    std::optional<Request> read_request();
    void send_response(Response& res);
    void send_text(const string& msg);
    void send_file(Response& res);
    bool send_all(char* buffer, size_t chunk);
};