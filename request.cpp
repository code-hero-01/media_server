#include "header_files/request.h"

// constructor
Request::Request(const string& raw_request) {
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

                // trim \r from key
                if (!key.empty() && key.back() == '\r')
                    key.pop_back();
                
                // time trailing '\r' from val
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

FormData::FormData(const string& body) {
        // find first boundary and skip past it
        size_t pos = body.find("--");
        if (pos == string::npos) return;
        
        pos = body.find("\r\n");
        if (pos == std::string::npos) return;
        pos += 2;

        size_t header_end = body.find("\r\n\r\n");
        string header_block = body.substr(pos, header_end-pos);
        if (header_block.find("Content-Disposition") != string::npos) {
                name = get_param(header_block, "name");
                filename = get_param(header_block, "filename");
        }
        if ((pos = header_block.find("Content-Type:")) != string::npos) {
                size_t end_pos = header_block.find("\r\n", pos);
                pos += 13;  // Skip "Content-Type:"
                content_type = header_block.substr(pos, end_pos-pos);
            
                // Remove leading space if present
                if (!content_type.empty() && content_type.front() == ' ')
                        content_type.erase(0, 1);
        }

        pos = header_end + 4;
        size_t end_pos = body.find("\r\n--", pos);
        if (end_pos != string::npos) 
                data = body.substr(pos, end_pos - pos);
        
        //std::cout << name << " " << filename << " " << content_type << "\n";
}

string FormData::get_param(const string& header, const string& param_name) {
        auto start_pos = header.find(param_name + "=\"");
        if (start_pos == string::npos) return "";
        start_pos += param_name.size() + 2; // skip param="
        
        auto end_pos = header.find("\"", start_pos);
        if (end_pos == std::string::npos) return "";

        return header.substr(start_pos, end_pos - start_pos);
}