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


struct UrlEncodedForm {
    std::unordered_map<string, string> fields;

    UrlEncodedForm(const string& body);

    bool has_field(const string& key) const;
    string get_field(const string& key) const;
};

string decode_url(const string& str);