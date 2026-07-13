#pragma once
#include <string>
using std::string;
#include <fstream>
#include <iterator>
#include <unordered_map>
#include <iostream>
#include <filesystem>
#include "logger.h"
#include "request.h"
namespace fs = std::filesystem;

struct ByteRange {
    size_t start;
    size_t end;
    bool valid = false;
};

namespace file_handler {    
    bool read_template(const string& file_path, string& page);
    void render_template(string& html, const string& key, const string& content);
    bool download_file(const string& file_path, const string& data);
    bool mkdir(const string& dir_path);
    bool delete_file(const string& path);
    bool rename_file(const string& old_path, const string& new_path);
    
    string get_content_type(const string& file_path);
    
    string decode_url(const std::string& str);
    string resolve_path(const string& url_path, const string& filesystem_root);
    size_t get_file_size(const string& file_path);
    ByteRange parse_range(const string& header, size_t file_size);
    
    string generate_dir_listing(const std::string& name, const string& root);
    string generate_breadcrumbs(const string& url_path);
};