#pragma once
#include <string>
using std::string;
#include <fstream>
#include <iterator>
#include <unordered_map>
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

namespace file_handler {
    bool serve_template(const string& filename, string& page);
    void render_template(string& html, const string& key, const string& content);
    bool serve_media(const string& filepath, string& content);
    string get_content_type(const string& filepath);
    string generate_content_list(const std::string& name);
    string url_decode(const std::string& str);
}