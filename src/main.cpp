#include "http_server.h"

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

int main(int argc, char* argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: media_server <optional: root_directory, default: ./media>\n";
        return 1;
    }
    
    int port_num = 6969;
    std::string port_str = std::to_string(port_num);
    std::string root = (argc == 2) ? argv[1] : "media";

    Server server(port_str, root, false);
    std::signal(SIGINT, server.handle_signal);
    server.start();

    return 0;
}

