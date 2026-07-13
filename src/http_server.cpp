// tcp server
#include "http_server.h"

using std::cerr;
#define BACKLOG 64

// constructor
Server::Server(string port, string dir_name, bool debug) 
    : PORT(port), ROOT(dir_name), debug(debug) {
        instance = this;
        
        // Prevent SIGPIPE from terminating the server when a client disconnects
        signal(SIGPIPE, SIG_IGN);
    }

void Server::start() {
    logger.start();
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
        
       logger.log("connected to client ", ipstr); 
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
        logger.log("handled request ", request_counter);
    }
}

void Server::handle_client(int client_fd, const string& client_ip) {
    HttpConnection conn(client_fd, client_ip, ROOT);
    // auto last_activity = std::chrono::steady_clock::now();
    while (true) {        
        pollfd pfd{};
        pfd.fd = client_fd;
        pfd.events = POLLIN;

        int ret = poll(&pfd, 1, timeout*1000);
        if (ret == -1) {
            if (errno == EINTR)
                continue;      // interrupted by signal

            logger.log("poll: ", strerror(errno), "\n");
            close(client_fd);
            logger.log("disconnected from client (", client_ip, ")");
            break;
        }
        
        if (ret == 0) { // timeout
            logger.log("idle time out after ", timeout, "s");
            close(client_fd);
            logger.log("disconnected from client (", client_ip, ")");
            break;
        } else {
            if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                close(client_fd);
                return;
            }
            if (!(pfd.revents & POLLIN))
                continue;

            std::optional<Request> req_opt = conn.read_request();
            if (!req_opt.has_value()) {
                close(client_fd);
                logger.log("disconnected from client (", client_ip, ")");
                break;
            }
            Request req = req_opt.value();
            //last_activity = std::chrono::steady_clock::now();   
                 
            bool keep_alive = true; // http 1.1 is keep alive by default

            if (req.has_header("Connection") && req.get_header("Connection") == "close")
                keep_alive = false;
            
            Response res = m_router.route(req, conn);
            
            if (keep_alive) {
                res.headers["Connection"] = "keep-alive";
                res.headers["Keep-Alive"] = "timeout=" + std::to_string(timeout);
            } else {
                res.headers["Connection"] = "close";
            }

            conn.send_response(res);
            //last_activity = std::chrono::steady_clock::now();

            if (!keep_alive) {
                close(client_fd);
                logger.log("disconnected from client (", client_ip, ")");
                break;
            }
        }
    }
}

void* Server::get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET)
        return &(((sockaddr_in*) sa)->sin_addr);
    else 
        return &(((sockaddr_in6*) sa)->sin6_addr);
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
    duration = std::chrono::duration<double>(end_time - Logger::start_time).count();
    std::cout << "Server ran for: " << duration << "s\n";
    std::cout << "Server handled: " << request_counter << " requests\n";
}



