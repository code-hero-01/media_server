#pragma once

#include <string>
#include <sstream>

using std::string;

struct Request {
    string method;
    string path;
    string version;

    Request(string html);
};