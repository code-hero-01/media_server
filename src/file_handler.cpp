#include "file_handler.h"

namespace file_handler {
    // read html text from an html file in template
    bool read_template(const string& file_path, string& page) {
        std::ifstream file(file_path);

        if (!file.is_open()) {
            logger.log("Error: Could not open the file: ", file_path, "\n");
            return false;
        }

        string line;
        while (std::getline(file, line)) {
            page += line + "\n";
        }

        file.close();
        return true;
    }

    // return html with the all instances of given key replaced by given content
    void render_template(
            string& html,
            const string& key,
            const string& content
        ) 
    {
        size_t pos = 0;
        while ((pos =  html.find(key, pos)) != string::npos) {
            html.replace(pos, key.size(), content);
            pos += content.size();
        }
    }

    
    bool download_file(const string& file_path, const string& data){
        std::ofstream file(file_path, std::ios::binary);

        if (!file.is_open()) {
            std::cerr << "Error: Could not open the file: " << file_path << "\n";
            return false;
        }

        uint32_t length = static_cast<uint32_t>(data.size());
        file.write(data.data(), length);
        
        file.close();
        return true;
    }

    bool mkdir(const string& dir_path) {
        fs::path path = dir_path;

        if (fs::create_directory(path)) {
            return true;
        } 
        return false;
    }

    bool delete_file(const string& path) {
        try {
            fs::remove(path); 
            return true;
        } catch (const fs::filesystem_error& e) {
            return false;
        }
    }

    bool rename_file(const string& old_path, const string& new_path) {
        try {
            fs::rename(old_path, new_path); 
            return true;
        } catch (const fs::filesystem_error& e) {
            return false;
        }
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
        
        size_t dot_pos = filepath.find_last_of('.');
        if (dot_pos == string::npos) 
            return "application/octet-stream"; 

        auto it = mime_types.find(filepath.substr(dot_pos));

        if (it != mime_types.end())
            return it->second;

        return "application/octet-stream";
    }

    string generate_dir_listing(const string& dir_path, const string& root) {
        string html;
        
        fs::path path = dir_path;
        
        if (fs::exists(path) && fs::is_directory(path)) {
            for (const auto& entry : fs::directory_iterator(path)) {
                string filename = entry.path().filename().string();
                
                fs::path rel = fs::relative(entry.path(), root);
                string filepath = "/" + rel.string();

                if (filename.find(":Zone.Identifier") != std::string::npos)
                    continue;
                
                html += "<div class=\"row\">";
                html += "<div class=\"filename\"><a href=\"" + filepath + "\"> > " + filename;
                if (entry.is_directory()) html += "/";
                html += "</a></div>";
                
                html += "<div class=\"menu-container\">";
                // popup-menu button
                html += "<button class=\"menu-btn\" aria-label=\"Toggle Menu\" " 
                        "onClick=\"toggleMenu(this)\">☰</button>";
                
                html += "<div class=\"menu\">";
                if (!entry.is_directory()) {    
                    // download button
                    html += "<form action=\"" + filepath + "/download\""
                            "method=\"post\">"
                            "<button type=\"submit\">"
                            "Download"
                            "</button>"
                            "</form>";
                }
                

                // rename button
                html += "<button type=\"button\" "
                        "onclick=\"renameFile('" + filepath + "', '" + filename + "'); event.preventDefault();\">"
                        "Rename"
                        "</button>";

                // delete button
                html += "<form action=\"" + filepath + "/delete\""
                        "method=\"post\""
                        "onsubmit=\"return confirm('Delete " + filename + "?');\">"
                        "<button type=\"submit\">"
                        "Delete"
                        "</button>"
                        "</form>";

                html += "</div>";  // menu
                html += "</div>";  // menu container
                html += "</div>";  // row
            }
        } else {
            std::cerr << "Directory: ./" << dir_path << " does not exist or is invalid.\n";
            return "";
        }
        return html;
    }

    string resolve_path(
        const string& url_path, 
        const string& fs_root) 
    {
        string decoded = decode_url(url_path);
        // logger.log("decoded = ", decoded);
        
        fs::path canonical_root = fs::weakly_canonical(fs_root);

        string url_prefix = fs_root;
        if (!url_prefix.starts_with('/')) 
            url_prefix = "/" + url_prefix;
        
        string rel = decoded;
        if (rel.starts_with(url_prefix)) rel = decoded.substr((url_prefix).size());
        if (!rel.empty() && rel.front() == '/')
            rel.erase(0, 1);
        fs::path relative(rel);
        // std::cout << relative.string() << "\n";
        fs::path requested = fs::weakly_canonical(fs::path(fs_root) / relative);
        // std::cout << requested.string() << "\n";

        if (requested.string().rfind(canonical_root.string(), 0) != 0) {
            throw std::runtime_error("path traversal - " + requested.string());
        }
        // std::cout << requested << "\n";
        return requested;   
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

    string generate_breadcrumbs(const string& url_path) {
        string html;
        // std::cout << url_path << "\n";
        std::istringstream ss(url_path);
        string segment;
        string current_path;

        html = R"(<a href="/">./ </a>)";

        while (std::getline(ss, segment, '/')) {
            if (segment.empty())
                continue;
            
            current_path += segment;

            html += "<a href=\"/" + current_path + "\">";
            html += segment + "/ ";
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