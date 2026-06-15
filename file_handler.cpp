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
    bool serve_media(const string& filepath, string& content) {
        string path = url_decode(filepath);
        std::ifstream file(path, std::ios::binary);

        if (!file.is_open()) {
            std::cerr << "Error: Could not open the file: " << path << "\n";
            return false;
        }

        content.assign(
            std::istreambuf_iterator<char>{file},
            std::istreambuf_iterator<char>{}
        );
        
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

    string generate_content_list(const string& dir_name) {
        string html = "<h2>./" + dir_name + "</h2>";
        html += "<ul>";
        fs::path path = "./" + dir_name;
        if (fs::exists(path) && fs::is_directory(path)) {
            for (const auto& entry : fs::directory_iterator(path)) {
                string filename = entry.path().filename().string();
                string filepath = entry.path().string().substr(1);
                if (filename.find(":Zone.Identifier") != std::string::npos)
                    continue;
                html += "<li><a href=\"" + filepath + "\">" + filename + "</a></li>";
            }
        } else {
            std::cerr << "Directory: ./" << dir_name << " does not exist or is invalid.\n";
            return "";
        }
        html += "</ul>";
        return html;
    }

    string url_decode(const std::string& str) {
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
}