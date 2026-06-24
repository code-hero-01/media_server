#pragma once
#include <string>
using std::string;
#include <fstream>
#include <iterator>
#include <unordered_map>
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

struct ByteRange {
    size_t start;
    size_t end;
    bool valid = false;
};

namespace file_handler {
    bool serve_template(const string& filename, string& page);
    void render_template(string& html, const string& key, const string& content);
    bool serve_file(const string& file_path, string& content, size_t start = 0, size_t end = SIZE_MAX);
    bool download_file(const string& file_path, const string& data);
    string get_content_type(const string& file_path);
    string generate_content_list(const std::string& name);
    string decode_url(const std::string& str);
    string resolve_path(const string& url_path);
    string generate_breadcrumbs(const string& dir_path);
    ByteRange parse_range(const string& header, size_t file_size);
    size_t get_file_size(const string& file_path);
}