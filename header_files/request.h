#pragma once

#include <string>
#include <sstream>
#include <unordered_map>
#include <iostream>

using std::string;

struct Request {
    string method;
    string path;
    string version;
    std::unordered_map<string, string> headers;
    string body;

    Request(const string& raw_request);

    bool has_header(const string& name) const;
    string get_header(const string& name) const;
};

struct FormData {
    string name;
    string filename;
    string content_type;
    string data;

    FormData(const string& body);
private:
    string get_param(const string& header, const string& param_name);
};