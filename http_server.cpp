// tcp server
#include "header_files/http_server.h"

using std::cerr;
#define BACKLOG 10

// constructor
Server::Server(string port) : port(port) { return; }

void Server::start() {
    addrinfo hints{}, *res; 
    hints.ai_family = AF_UNSPEC;    // either ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM;   // TCP
    hints.ai_flags = AI_PASSIVE;

    int status;
    if ((status = getaddrinfo(NULL, port.c_str(), &hints, &res)) != 0) {
        cerr << "getaddrinfo: " << gai_strerror(status) << "\n";
        exit(1);
    }

    // loop through the results linked list and bind to the first one that we can
    addrinfo* p;
    for (p = res; p != nullptr; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            cerr << "server: socket " << std::strerror(errno) << "\n";
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            cerr << "setsockopt: " << std::strerror(errno) << "\n";;
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            cerr << "server: bind " << std::strerror(errno) << "\n";;
            continue;
        }

        break;
    }
    freeaddrinfo(res);
    
    if (p == NULL) {
        cerr << "server failed to bind...\n";
        exit(1);
    }
}

void Server::start_listening() {
    if (listen(sockfd, BACKLOG) == -1) {
        cerr << "listen: " << std::strerror(errno) << "\n";
        exit(1);
    }
    std::cout << "Listening on port: " << port << "\n";
    std::cout << "Waiting for connections...\n";

    sockaddr_storage their_addr;
    socklen_t addr_size;
    while(true) {
        addr_size = sizeof (their_addr);
        int client_fd = accept(sockfd, (sockaddr*) &their_addr, &addr_size);
        if (client_fd == -1) {
            cerr << "accept: " << std::strerror(errno) << "\n";
            continue;
        }

        char ipstr[INET6_ADDRSTRLEN];
        inet_ntop(their_addr.ss_family, get_in_addr((sockaddr*) &their_addr), ipstr, sizeof(ipstr));
        cerr << "Server connected to: " << ipstr << "\n";
         
        handle_client(client_fd, ipstr);
    }
}

void Server::handle_client(int client_fd, char* ipstr) {
        std::string receive_msg;
        char buffer[1024];
        while (true) {
            int bytes = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes == -1) {
                cerr << "recv: " << std::strerror(errno) << "\n";
                exit(1);
            }
            if (bytes == 0) {
                std::cout << "Client " << ipstr << " closed connection\n";
                break;    
            }
            receive_msg.append(buffer, bytes);
            if (receive_msg.find("\r\n\r\n") != std::string::npos) // break if we reach end of http request
                break;
        }
        Request req(receive_msg);
        std::cout << "Request type: " << req.method << "\n";
        std::cout << "Request path: " << req.path << "\n";
        std::cout << "Request version: " << req.version << "\n";
                
        string response_msg = parse_request(req);
        send_msg(client_fd, response_msg, ipstr);
        
        close(client_fd);
}

void* Server::get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET)
        return &(((sockaddr_in*) sa)->sin_addr);
    else 
        return &(((sockaddr_in6*) sa)->sin6_addr);
}

void Server::send_msg(int client_fd, string msg, char* ipstr) {
    if (send(client_fd, msg.c_str(), msg.size(), 0) == -1) {
            cerr << "send: " << std::strerror(errno) << "\n";
        } else {
            cerr << "Message sent to client: " << ipstr << "\n";
        }
}

string Server::parse_request(Request req) {
    if (req.method != "GET") return get_html("405");
    if (req.path == "/") return get_html("home");
    else return get_html(req.path.substr(1));
}

string Server::get_html(string filename) {
    std::ifstream file("templates/" + filename + ".html");

    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file" << "\n";
    }

    string content;
    string line;
    while (std::getline(file, line)) {
        content += line + "\n";
    }

    file.close();

    string html = "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + std::to_string(content.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            content;
    
    return html;
}
