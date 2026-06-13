#include "header_files/request.h"

// constructor
Request::Request(string line) {
        std::istringstream iss(line);
        iss >> method
            >> path
            >> version;
    }