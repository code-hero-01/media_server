#pragma once
#include <string>
using std::string;
#include <fstream>
#include <iterator>
#include <unordered_map>
#include <iostream>
#include <filesystem>
#include "logger.h"
namespace fs = std::filesystem;

struct ByteRange {
    size_t start;
    size_t end;
    bool valid = false;
};

class FileHandler {
private:
    const string ROOT;
    const string ROOT_PATH;
public:
    FileHandler (const string& root_dir);
    
    bool serve_template(const string& file_path, string& page);
    void render_template(string& html, const string& key, const string& content);
    bool serve_file(const string& file_path, string& content, size_t start = 0, size_t end = SIZE_MAX);
    bool download_file(const string& file_path, const string& data);
    bool mkdir(const string& dir_path);
    
    string get_content_type(const string& file_path);
    
    string decode_url(const std::string& str);
    string resolve_path(const string& url_path, const string& filesystem_root);
    static size_t get_file_size(const string& file_path);
    static ByteRange parse_range(const string& header, size_t file_size);
    
    string generate_dir_listing(const std::string& name);
    string generate_breadcrumbs(const string& url_path);
};