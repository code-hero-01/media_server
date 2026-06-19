#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <numeric>
#include <mutex>
#include <algorithm>

using namespace std::chrono_literals;

#define PORT "6969"
#define SERVER_IP "127.0.0.1" 

void worker(
    int thread_id, 
    addrinfo* res, 
    int num_requests, 
    std::atomic<int>& failed_count,
    std::vector<double>& latencies,
    std::mutex& latency_mutex,
    std::atomic<bool>& go
) 
    {
    while (!go) {
        std::this_thread::yield();
    }
    std::cerr << "Worker: " << thread_id << " woke up\n";

    for (int i = 0; i < num_requests; i++) {
        auto req_start = std::chrono::steady_clock::now();

        int sockfd;
        if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
            std::cerr << "Worker: " << thread_id << " socket " << std::strerror(errno) << "\n";
            failed_count++;
            continue;;
        }
        
        //std::cerr << "Worker: " << thread_id << " connecting...\n";
        if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
            std::cerr << "Worker: " << thread_id << " connection failed: " << std::strerror(errno) << "\n";
            close(sockfd);
            failed_count++;
            continue;
        }

        //std::cerr << "Worker: " << thread_id << "Successfully connected to " << SERVER_IP << "\n";

        // std::string msg =
        //     "GET / HTTP/1.1\r\n"
        //     "Host: localhost\r\n"
        //     "Connection: close\r\n"
        //     "\r\n";
        std::string msg =
            "GET /media/makoto%20hittin%20that%20shi.mp4 HTTP/1.1\r\n"
            "Host: localhost\r\n"
            "Range: bytes=0-\r\n"
            "Connection: close\r\n"
            "\r\n";
        if (send(sockfd, msg.c_str(), msg.size(), 0) == -1) {
            std::cerr << "Worker: " << thread_id << " send: " << std::strerror(errno) << "\n";
            failed_count++;
            close(sockfd);
            continue;
        };
        //std::cerr << "Worker: " << thread_id << " send message\n";

        size_t total_bytes = 0;
        char buffer[4096];
        while (true) {
            ssize_t bytes = recv(sockfd, buffer, sizeof(buffer), 0);

            if (bytes == 0)
                break;
            if (bytes == -1) {
                std::cerr << "recv interrupted";
                failed_count++;
                break;
            }
            total_bytes += bytes;
        }
        //std::cerr << "Worker: " << thread_id << " received message from: " << SERVER_IP << "\n";
        
        auto req_end = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(req_end - req_start).count();
        
        {
            std::lock_guard<std::mutex> lock(latency_mutex);
            latencies.push_back(ms);
        }
        close(sockfd);

        std::cerr << "worker " << thread_id << " Bytes received: " << total_bytes << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: benchmark <number of clients> <number of requests per client>\n";
        return 1;
    }
    const int NUM_CLIENTS = std::stoi(argv[1]);
    const int NUM_REQUESTS = std::stoi(argv[2]);
    
    addrinfo hints{}, *res; 
    hints.ai_family = AF_UNSPEC;    // either ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM;   // TCP
    hints.ai_flags = AI_NUMERICHOST;  // do not resolve DNS, expect numeric IP

    int status;
    if ((status = getaddrinfo(SERVER_IP, PORT, &hints, &res)) != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << "\n";
        return 1;
    }

    std::atomic<int> failed_count{0};

    unsigned int num_threads = NUM_CLIENTS;
    std::vector<std::thread> threads;

    std::vector<double> latencies;
    std::mutex latency_mutex;
    std::atomic<bool> go = false;
   

    for (unsigned int i = 0; i < num_threads; ++i) {
        threads.emplace_back(
            worker, 
            i, 
            res, 
            NUM_REQUESTS, 
            std::ref(failed_count),
            std::ref(latencies),
            std::ref(latency_mutex),
            std::ref(go)
        );
    }
    std::this_thread::sleep_for(1s);
    auto start = std::chrono::steady_clock::now();
    go = true; // start all threads concurrently

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    freeaddrinfo(res);

    auto end = std::chrono::steady_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();

    std::sort(latencies.begin(), latencies.end());

    double min_latency = latencies.front();
    double max_latency = latencies.back();
    double avg_latency = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
    double median_latency = latencies[latencies.size() / 2];
    double p95_latency = latencies[static_cast<size_t>(latencies.size() * 0.95)];
    double p99_latency = latencies[static_cast<size_t>(latencies.size() * 0.99)];

    int total_requests = NUM_CLIENTS * NUM_REQUESTS;
    std::cout << "Number of Clients: " << NUM_CLIENTS << "\n";
    std::cout << "Requests per client: " << NUM_REQUESTS << "\n";
    std::cout << "Total Requests: " << total_requests << "\n";
    std::cout << "Total Time: " << seconds << " s\n";
    std::cout << "Req/s: " << total_requests / seconds << "\n";

    std::cout << "Minimum Latency: " << min_latency << "ms\n";
    std::cout << "Average Latency: " << avg_latency << "ms\n";
    std::cout << "Median Latency: " << median_latency << "ms\n";
    std::cout << "P95 Latency: " << p95_latency << "ms\n";
    std::cout << "P99 Latency: " << p99_latency << "ms\n";
    std::cout << "Maximum Latency: " << max_latency << "ms\n";
    
    std::cout << "Failed: " << failed_count << "\n";
    
    return 0;
}