#include "header_files/request.h"

// constructor
Request::Request(string raw_request) {
        std::istringstream stream(raw_request);

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

                // remove trailing '\r'
                if (!val.empty() && val.back() == '\r')
                        val.pop_back();
                headers[key] = val;
        }
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
