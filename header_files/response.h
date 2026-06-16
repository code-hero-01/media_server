#pragma once

#include <string>
using std::string;
#include "request.h"

class Response{
private:
    int status;
    string content_type;
    string body;
public:
    std::unordered_map<string, string> headers;
    Response(
        const int status, 
        const string& content_type, 
        const string& body);

    string serialize();
};