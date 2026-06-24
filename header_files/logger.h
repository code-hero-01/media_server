#pragma once

#include <iostream>
#include <string>
using std::string;
#include <mutex>
#include <thread>
#include <chrono>
using namespace std::chrono_literals;

class Logger{
private:
    std::mutex log_mutex;
public:
    inline static std::chrono::steady_clock::time_point start_time;

    void start() {
        start_time = std::chrono::steady_clock::now();
    }
    
    template <typename... Args>
    void log(const Args&... args) {
        std::lock_guard<std::mutex> lock(log_mutex);

        auto now = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(now - start_time).count();

        std::cerr << "["
            << ms
            << "ms]["
            << std::this_thread::get_id()
            << "] ";

        (std::cerr << ... << args);

        std::cerr << '\n';
    }
};

inline Logger logger;