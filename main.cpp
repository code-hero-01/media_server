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

int main(void) {
    int port_num = 6969;
    std::string port_str = std::to_string(port_num);
    Server server(port_str);
    server.start();
    server.start_listening();
    return 0;
}

