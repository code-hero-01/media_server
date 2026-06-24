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

class HttpConnection {
private:
    int client_fd;
    const string& client_ip;
public:
    HttpConnection(int client_fd, const string& client_ip);
    std::optional<Request> read_request();
    void send_response(const string& msg);
};