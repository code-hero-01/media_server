#include "header_files/file_handler.h"

namespace file_handler {
    // read html text from an html file in template
    bool serve_template(const string& filename, string& page) {
        string filepath = "templates/" + filename + ".html";
        std::ifstream file(filepath);

        if (!file.is_open()) {
            std::cerr << "Error: Could not open the file: " << filepath << "\n";
            return false;
        }

        string line;
        while (std::getline(file, line)) {
            page += line + "\n";
        }

        file.close();
        return true;
    }

    // return html with the key replaced by content
    void render_template(
            string& html,
            const string& key,
            const string& content
        ) 
    {
        size_t pos = html.find(key);
        if (pos != string::npos) {
            html.replace(pos, key.size(), content);
        }
    }

    // read binary data from a file in static
    bool serve_file(const string& filepath, string& content, size_t start, size_t end) {
        string path = decode_url(filepath);
        std::ifstream file(path, std::ios::binary);

        if (!file.is_open()) {
            std::cerr << "Error: Could not open the file: " << path << "\n";
            return false;
        }

        size_t file_size = get_file_size(path);

        if (end == SIZE_MAX)
            end = file_size - 1;

        if (start > end || end >= file_size)
            return false;

        size_t length = end - start + 1;
        content.resize(length);
        file.seekg(start);
        
        file.read(content.data(), length);
        
        return true;
    }


    string get_content_type(const string& filepath) {
        static const std::unordered_map<string, string> mime_types = {
            {".html", "text/html"},
            {".css",  "text/css"},
            {".gif",  "image/gif"},
            {".png",  "image/png"},
            {".jpg",  "image/jpeg"},
            {".jpeg", "image/jpeg"},
            {".js",   "application/javascript"},
            {".mp3",  "audio/mpeg"},
            {".mp4",  "video/mp4"}
        };
        
        auto ext = filepath.substr(filepath.find_last_of('.'));

        auto it = mime_types.find(ext);

        if (it != mime_types.end())
            return it->second;

        return "application/octet-stream";
    }

    string generate_content_list(const string& dir_path) {
        string html;
        html += "<ul>";
        fs::path path = dir_path;
        if (fs::exists(path) && fs::is_directory(path)) {
            for (const auto& entry : fs::directory_iterator(path)) {
                string filename = entry.path().filename().string();
                string filepath = entry.path().string().substr(1);
                if (filename.find(":Zone.Identifier") != std::string::npos)
                    continue;
                
                if (entry.is_directory()) { // indent subdirectories
                    html += "<li style=\"margin-left: 40px;\"><a href=\"" + filepath + "\">" + filename + "</a></li>";
                }
                else {
                    html += "<li><a href=\"" + filepath + "\">" + filename + "</a></li>";
                }
            }
        } else {
            std::cerr << "Directory: ./" << dir_path << " does not exist or is invalid.\n";
            return "";
        }
        html += "</ul>";
        return html;
    }

    string resolve_path(const string& url_path) {
        string decoded = decode_url(url_path);
        return "." + decoded;
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

    string generate_breadcrumbs(const string& dir_path) {
        string html;
        std::istringstream ss(dir_path.substr(1));
        string segment;
        string current_path;

        html = R"(<a href="/">Home</a>)";

        while (std::getline(ss, segment, '/')) {
            if (segment.empty())
                continue;
            
            current_path += "/" + segment;

            html += " / ";
            html += "<a href=\"" + current_path + "\">";
            html += segment;
            html += "</a>";
        }

        return html;
    }

    // parse "bytes=num-"
    ByteRange parse_range(const string& header, size_t file_size) {
        ByteRange range;     

        string value = header.substr(6); // without "bytes="
        
        // get num
        size_t dash_pos = value.find('-');
        if (dash_pos == string::npos)
            return range;
        
        string start_str = value.substr(0, dash_pos);
        if (start_str.empty())
            return range;
        range.start = stoull(start_str);
        
        if (range.start >= file_size)
            return range;

        range.end = file_size - 1;
        range.valid = true;
        return range;   
    }

    size_t get_file_size(const string& file_path) {
        return  std::filesystem::file_size(file_path);
    }
}