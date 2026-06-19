#include "header_files/http_server.h"

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
#include <thread>
#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>
#include <vector>
using namespace std::chrono_literals;

int main() {
    int port_num = 6969;
    std::string port_str = std::to_string(port_num);
    
    Server server(port_str, "media", false);
    std::signal(SIGINT, server.handle_signal);
    server.start();

    return 0;
}

