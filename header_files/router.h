#pragma once

#include <iostream>
#include <string>
#include "request.h"
#include "response.h"
#include "file_handler.h"
#include "logger.h"

using std::string;

class Router {
private:
    const string& ROOT;
    FileHandler file_handler{ROOT};

    Response handle_home(const Request& req);
    Response handle_template(const Request& req);
    Response handle_static(const Request& req);
    Response handle_media(const Request& req, const string& fs_root);
    Response handle_upload(const Request& req);
    Response handle_mkdir(const Request& req);
public:
    Router(const string& dir_name);
    Response route(const Request& req);
};