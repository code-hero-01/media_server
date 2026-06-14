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
        std::string received_msg;
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
            received_msg.append(buffer, bytes);
            if (received_msg.find("\r\n\r\n") != std::string::npos) // break if we reach end of http request
                break;
        }
        
        Request req(received_msg);
        
        if (debug == true) {
            std::cout << received_msg << "\n\n";
            
            // std::cout << "Request type: " << req.method << "\n";
            // std::cout << "Request path: " << req.path << "\n";
            // std::cout << "Request version: " << req.version << "\n";   
        }
                
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

string Server::parse_request(const Request& req) {
    if (req.method != "GET") {
        return make_response(
            405,
            "text/html",
            "<h1>405 Method Not Allowed</h1>"
        );
    }

    if (req.version != "HTTP/1.1")
        {
            return make_response(
                505,
                "text/html",
                "<h1>505 HTTP Version Not Supported</h1>"
            );
        }

    string page;
    string content_type;

    if (req.path == "/") {
        content_type = "text/html";
        page = serve_template("home");
    }
    else if (req.path.starts_with("/static/")) {
        content_type = "image/gif";
        page = serve_static(req.path);
    }
    else {
        content_type = "text/html";
        page = serve_template(req.path.substr(1));
    }

    if (page.empty()) {
        return make_response(
            404,
            "text/html",
            "<h1>404 Not Found</h1>"
        );
    }

    return make_response(200, content_type, page);
}

std::string Server::make_response(
    int status,
    string content_type,
    const std::string& body)
{
    std::string status_line;

    if (status == 200)
        status_line = "HTTP/1.1 200 OK\r\n";
    else if (status == 404)
        status_line = "HTTP/1.1 404 Not Found\r\n";
    else if (status == 405)
        status_line = "HTTP/1.1 405 Method Not Allowed\r\n";

    return status_line +
           "Content-Type: " + content_type + "\r\n"
           "Content-Length: " + std::to_string(body.size()) + "\r\n"
           "Connection: close\r\n"
           "\r\n" +
           body;
}

// return html text from an html file in template
string Server::serve_template(string filename) {
    string filepath = "templates/" + filename + ".html";
    std::ifstream file(filepath);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file: " << filepath << "\n";
    }

    string content;
    string line;
    while (std::getline(file, line)) {
        content += line + "\n";
    }

    file.close();
    
    return content;
}

// return binary data from a file in static
string Server::serve_static(string filepath) {
    std::ifstream file(filepath.substr(1), std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file: " << filepath << "\n";
    }

    string content(
        std::istreambuf_iterator<char>{file},
        std::istreambuf_iterator<char>{}
    );
    
    return content;
}

string Server::get_content_type(string filepath) {
    static const std::unordered_map<string, string> mime_types = {
        {".html", "text/html"},
        {".css",  "text/css"},
        {".gif",  "image/gif"},
        {".png",  "image/png"},
        {".jpg",  "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".js",   "application/javascript"},
        {".mp3",  "audio/mpeg"},
        {".mp4",  "video/mp4"}
    };

    auto ext = filepath.substr(filepath.find_last_of('.'));

    auto it = mime_types.find(ext);

    if (it != mime_types.end())
        return it->second;

    return "application/octet-stream";
}
