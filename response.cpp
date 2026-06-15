#include "header_files/response.h"

// constructor
Response::Response(
    const int status, 
    const string& content_type, 
    const string& body) 
    : status(status), content_type(content_type), body(body) {}

string Response::serialize() {
     std::string reason;

    switch (status)
    {
        case 200: reason = "OK"; break;
        case 404: reason = "Not Found"; break;
        case 405: reason = "Method Not Allowed"; break;
        case 505: reason = "HTTP Version Not Supported"; break;
        default:  reason = "Unknown";
    }

    return
        "HTTP/1.1 " + std::to_string(status) + " " + reason + "\r\n"
        "Content-Type: " + content_type + "\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n\r\n" +
        body;  
}