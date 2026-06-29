#include "header_files/request.h"

// constructor
Request::Request(const string& raw_headers) {
        std::istringstream stream(raw_headers);
        // std::cout << raw_headers << "\n";

        string line;
        std::getline(stream, line);

        std::istringstream request_line(line);
 
        request_line >> method
                >> path
                >> version;

        // headers
        while (std::getline(stream, line)) {
                auto pos = line.find(':');
                if (pos == string::npos) 
                        continue;       
                string key = line.substr(0, pos);
                string val = line.substr(pos + 1);
                // remove leading space
                if (!val.empty() && val.front() == ' ')
                        val.erase(0, 1);

                // trim \r from key
                if (!key.empty() && key.back() == '\r')
                    key.pop_back();
                
                // time trailing '\r' from val
                if (!val.empty() && val.back() == '\r')
                    val.pop_back();
                headers[key] = val;
        }

        // for (const auto& [key, val] : headers) {
        //         std::cout << key << ": " << val << "\n";
        // }
}

bool Request::has_header(const string& name) const {
    return headers.contains(name);
}

string Request::get_header(const string& name) const {
        auto it = headers.find(name);

        if (it != headers.end())
                return it->second;
        
        return "";
}

UrlEncodedForm::UrlEncodedForm(const string& body) {
        std::istringstream stream(body);
        string pair;
        while (std::getline(stream, pair)) {
                size_t pos = pair.find('=');
                if (pos == string::npos) continue;

                fields[decode_url(pair.substr(0, pos))] = pair.substr(pos+1);
        }
}

bool UrlEncodedForm::has_field(const string& key) const {
        return fields.contains(key);
}

string UrlEncodedForm::get_field(const string& key) const {
        auto it = fields.find(key);

        if (it != fields.end())
                return it->second;
        
        return "";
}

string decode_url(const string& str) {
    std::string result;

    for (size_t i = 0; i < str.size(); i++) {
        if (str[i] == '%' && i + 2 < str.size()) {
            std::string hex = str.substr(i + 1, 2);
            char decoded_char = static_cast<char>(std::stoi(hex, nullptr, 16));
            result += decoded_char;
            i += 2;
        }
        else if (str[i] == '+')
            result += ' ';
        else
            result += str[i];
    }

    return result;
}
