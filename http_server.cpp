// tcp server
#include "header_files/http_server.h"

using std::cerr;
#define BACKLOG 64

// constructor
Server::Server(string port, string dir_name, bool debug) 
    : PORT(port), DIR_NAME(dir_name), debug(debug) {
        instance = this;
        
        // Prevent SIGPIPE from terminating the server when a client disconnects
        signal(SIGPIPE, SIG_IGN);
    }

void Server::start() {
    std::cerr << "Media_Server waking up...\n";

    addrinfo hints{}, *res; 
    hints.ai_family = AF_UNSPEC;    // either ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM;   // TCP
    hints.ai_flags = AI_PASSIVE;

    int status;
    if ((status = getaddrinfo(NULL, PORT.c_str(), &hints, &res)) != 0) {
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

    
    std::vector<std::thread> threads;

    start_time = std::chrono::steady_clock::now();
    
    if (listen(sockfd, BACKLOG) == -1) {
        cerr << "listen: " << std::strerror(errno) << "\n";
        exit(1);
    }
    
    std::thread listener_thread(&Server::listener, this);

    std::cout << "Listening on port: " << PORT << "\n";
    std::cout << "Waiting for connections...\n";

    for (unsigned int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(
            &Server::worker,
            this 
        );
    }

    if (listener_thread.joinable()) 
        listener_thread.join();
    
    for (auto& t : threads) {
        if (t.joinable()) {
            //std::cerr << "Worker (" << worker_id << ") going to sleep...\n";
            t.join();
        }
    }

    shutdown();
}

std::mutex q_mutex;

void Server::listener() {
    sockaddr_storage their_addr;
    socklen_t addr_size;
    while (running) {
        addr_size = sizeof (their_addr);
        int client_fd = accept(sockfd, (sockaddr*) &their_addr, &addr_size);
        if (client_fd == -1) {
            if (!running) break;
            
            cerr << "accept: " << std::strerror(errno) << "\n";
            continue;
        }
        
        if (!running) {
            close(client_fd);
            break;
        }

        char ipstr[INET6_ADDRSTRLEN];
        inet_ntop(their_addr.ss_family, get_in_addr((sockaddr*) &their_addr), ipstr, sizeof(ipstr));
        
        {
            std::lock_guard<std::mutex> lock(q_mutex);
            Client client(client_fd, ipstr);
            client_q.push(client);
        }
        cv.notify_one();
        
       log(string("connected to client (") + ipstr + ")"); 
    } 
}

void Server::worker() {
    while(running) { 
        Client client;
        {
            std::unique_lock<std::mutex> lock(q_mutex);
            
            cv.wait(lock, [&] { return !client_q.empty() || !running; });
            
            if (!running && client_q.empty()) return; 
            
            client = client_q.front();
            client_q.pop();
        }
        
        handle_client(client.fd, client.ip);
        
        request_counter++;
        log(string("handled request ") + to_string(request_counter));
    }
}

void Server::handle_client(int client_fd, char* ipstr) {
    std::this_thread::sleep_for(1s);
    
    std::string received_msg;
    char buffer[1024];
    while (true) {
        int bytes = recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes == -1) {
            cerr << "recv: " << std::strerror(errno) << "\n";
            close(client_fd);
            return;
        }
        if (bytes == 0) {
            std::cout << "Client (" << ipstr << ") closed connection\n";
            break;    
        }
        received_msg.append(buffer, bytes);
        if (received_msg.find("\r\n\r\n") != std::string::npos) // break if we reach end of http request
            break;
    }
    
    Request req(received_msg);
    
    if (debug == true) {
        std::cerr << "Client (" << ipstr << ") sent: \n\n";
        std::cerr << received_msg << "\n\n";
        
        // std::cout << "Request type: " << req.method << "\n";
        // std::cout << "Request path: " << req.path << "\n";
        // std::cout << "Request version: " << req.version << "\n";   
    }
            
    Response res = router(req);
    string response_msg = res.serialize();
    send_msg(client_fd, response_msg, ipstr);
    
    close(client_fd);
    log(string("disconnected from client (") + ipstr + ")");
}

void* Server::get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET)
        return &(((sockaddr_in*) sa)->sin_addr);
    else 
        return &(((sockaddr_in6*) sa)->sin6_addr);
}

void Server::send_msg(int client_fd, const string& msg, char* ipstr) {
    size_t total = 0;
    //std::cerr << msg << "\n";
    while (total < msg.size()) {
        ssize_t bytes_sent =
            send(client_fd,
                msg.c_str() + total,
                msg.size() - total,
                0);

        if (bytes_sent == -1) {
            if (errno == EPIPE || errno == ECONNRESET) {
                cerr << "Client (" << ipstr << ") disconnected\n";
            } else {
                cerr << "send: " << std::strerror(errno) << "\n";
            }
            return;
        }

        total += bytes_sent;
    }

    log(string("sent message to client (") + ipstr + ")");
}

Response Server::router(const Request& req)
{
    if (req.method != "GET") {
        return Response(
            405,
            "text/html",
            "<h1>405 Method Not Allowed</h1>"
        );
    }

    if (req.version != "HTTP/1.1") {
        return Response(
            505,
            "text/html",
            "<h1>505 HTTP Version Not Supported</h1>"
        );
    }

    // homepage
    if (req.path == "/") {
        string page;
        
        if (!file_handler::serve_template("./home", page)) 
            return Response(404, "text/html", "<h1>404 Not Found</h1>");
        
        string content_list = file_handler::generate_content_list("./" + DIR_NAME);    
        file_handler::render_template(page, "{{CONTENT_LIST}}", content_list);
        file_handler::render_template(page, "{{DIR_NAME}}", "./" + DIR_NAME);
        return Response(200, "text/html", page);
    }

    // static file request
    if (req.path.starts_with("/static/")) {
        string path = file_handler::resolve_path(req.path);
        string content;
            if (!file_handler::serve_file(path, content)) 
                return Response(404, "text/html", "<h1>404 Not Found</h1>");

        return Response(200, file_handler::get_content_type(path), content);
    }

    // file or folder request
    if (req.path.starts_with("/" + DIR_NAME)) {
        return handle_media_route(req);
    }

    // template routes (/about, /contact, etc.)
    string page;
    if (!file_handler::serve_template(req.path.substr(1), page))
        return Response(404, "text/html", "<h1>404 Not Found</h1>");

    return Response(200, "text/html", page);
}


Response Server::handle_media_route(const Request& req) {
    string path = file_handler::resolve_path(req.path);
    
    if (std::filesystem::is_directory(path)) {  // if directory is requested
        string page;
        if (!file_handler::serve_template("./subdir", page)) 
            return Response(404, "text/html", "<h1>404 Not Found</h1>");
        
        string content_list = file_handler::generate_content_list(path);
        file_handler::render_template(page, "{{CONTENT_LIST}}", content_list);
        
        string breadcrumbs = file_handler::generate_breadcrumbs(path);
        file_handler::render_template(page, "{{BREADCRUMBS}}", breadcrumbs);

        return Response(200, "text/html", page);
    }    
    else {  // if file is requested
        string content;
        if (req.has_header("Range")) { // partial response
            size_t file_size = file_handler::get_file_size(path);
            ByteRange range = file_handler::parse_range(req.get_header("Range"), file_size);
            
            if (!range.valid || !file_handler::serve_file(path, content, range.start, range.end)) 
                return Response(404, "text/html", "<h1>404 Not Found</h1>");
            
            Response res(206, file_handler::get_content_type(path), content);

            res.headers["Accept-Ranges"] = "bytes";
            res.headers["Content-Range"] =
                "bytes "
                + std::to_string(range.start)
                + "-"
                + std::to_string(range.end)
                + "/"
                + std::to_string(file_size);

            return res;
        }   
        else {

            if (!file_handler::serve_file(path, content)) 
                return Response(404, "text/html", "<h1>404 Not Found</h1>");

            return Response(200, file_handler::get_content_type(path), content);
        }
    }
}

bool Server::check(int result, string& err_msg) {
    if (result < 0) {
        cerr << err_msg << " " << strerror(errno) << "\n";
        return true; // error occured
    }

    return false; // no error
}

std::mutex log_mutex;
void Server::log(string msg) const {
    std::lock_guard<std::mutex> lock(log_mutex);

    auto now = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(now - start_time).count();

    std::cerr << "["
              << ms
              << "ms]["
              << std::this_thread::get_id()
              << "] "
              << msg
              << "\n";
}

// handle ctrl-c signal
void Server::handle_signal(int signal_num) {
    if (signal_num == SIGINT && instance != nullptr) {
        std::cout << "\n[Ctrl+C detected! Initiating graceful shut down...]\n";
        instance->running = false;
        ::shutdown(instance->sockfd, SHUT_RDWR);
        instance->cv.notify_all();
        close(instance->sockfd);
    } 
}

void Server::shutdown() {
    std::cout << "Media_Server shutting down...\n";
    auto end_time = std::chrono::steady_clock::now();
    duration = std::chrono::duration<double>(end_time - start_time).count();
    std::cout << "Server ran for: " << duration << "s\n";
    std::cout << "Server handled: " << request_counter << " requests\n";
}



