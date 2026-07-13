#pragma once

#include <string>
using std::string;
#include "request.h"

struct Response{
    int status;
    string headers_text;
    string content_type;
    bool is_file = false;
    string text;
    size_t content_length;
    string path;
    size_t start = 0, end = SIZE_MAX;

    std::unordered_map<string, string> headers;
    
    Response() = default; // default construction
    Response(
        const int status, 
        const string& content_type, 
        const string& text);

    string serialize();
};