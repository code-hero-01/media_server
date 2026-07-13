#pragma once

#include <iostream>
#include <string>
#include "request.h"
#include "response.h"
#include "file_handler.h"
#include "logger.h"
#include "http_connection.h"

using std::string;

class Router {
private:
    const string& ROOT;
  
    Response handle_home(const Request& req);
    Response handle_template(const Request& req);
    Response handle_static(const Request& req);
    Response handle_media(const Request& req, const string& fs_root);
    Response handle_upload(const Request& req, HttpConnection& conn);
    Response handle_mkdir(const Request& req);
    Response handle_delete(const Request& req);
    Response handle_rename(const Request& req);
public:
    Router(const string& dir_name);
    Response route(const Request& req, HttpConnection& conn);
};