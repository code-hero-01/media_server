#pragma once

#include <string>
#include <sstream>
#include <unordered_map>
#include <iostream>

using std::string;

class Request {
public:
    string method;
    string path;
    string version;
    std::unordered_map<string, string> headers;

    Request(string raw_request);

    bool has_header(const string& name) const;
    string get_header(const string& name) const;
};