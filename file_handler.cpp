#include "header_files/file_handler.h"

FileHandler::FileHandler(const string& root_dir) :
    ROOT(root_dir), ROOT_PATH("./" + root_dir) {}

// read html text from an html file in template
bool FileHandler::serve_template(const string& file_path, string& page) {
    std::ifstream file(file_path);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file: " << file_path << "\n";
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
void FileHandler::render_template(
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

// read binary data from a file in static
bool FileHandler::serve_file(const string& file_path, string& content, size_t start, size_t end) {
    string path = decode_url(file_path);
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

bool FileHandler::download_file(const string& file_path, const string& data){
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

bool FileHandler::mkdir(const string& dir_path) {
    fs::path path = dir_path;

    if (fs::create_directory(path)) {
        return true;
    } 
    return false;
}

bool FileHandler::delete_file(const string& path) {
    try {
        fs::remove(path); 
        return true;
    } catch (const fs::filesystem_error& e) {
        return false;
    }
}

bool FileHandler::rename_file(const string& old_path, const string& new_path) {
    try {
        fs::rename(old_path, new_path); 
        return true;
    } catch (const fs::filesystem_error& e) {
        return false;
    }
}

string FileHandler::get_content_type(const string& filepath) {
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

string FileHandler::generate_dir_listing(const string& dir_path) {
    string html;
    html += "<ul>";
    fs::path path = dir_path;
    
    if (fs::exists(path) && fs::is_directory(path)) {
        for (const auto& entry : fs::directory_iterator(path)) {
            string filename = entry.path().filename().string();
            
            fs::path rel = fs::relative(entry.path(), ROOT_PATH);
            string filepath = "/" + ROOT + "/" + rel.string();

            if (filename.find(":Zone.Identifier") != std::string::npos)
                continue;
            
            if (entry.is_directory()) { // indent subdirectories
                html += "<li style=\"display:flex; gap:10px; align-items:center; margin-left: 40px;\"><a href=\"" + filepath + "\">" + filename + "</a>";
            }
            else {
                html += "<li style=\"display:flex; gap:10px; align-items:center;\"><a href=\"" + filepath + "\">" + filename + "</a>";
            }

            // rename button
            html+= "<button href=\"#\""
                    "type=\"button\""
                    "onclick=\"renameFile('" + filepath + "'); event.preventDefault();\">"
                    "Rename"
                    "</button>";

            // delete button
            html+= "<form action=\"" + filepath + "/delete\""
                    "method=\"post\""
                    "onsubmit=\"return confirm('Delete " + filename + "?');\">"
                    "<button type=\"submit\">Delete</button>"
                    "</form>";
            html += "</li>"; 
        }
    } else {
        std::cerr << "Directory: ./" << dir_path << " does not exist or is invalid.\n";
        return "";
    }
    html += "</ul>";
    return html;
}

string FileHandler::resolve_path(
    const string& url_path, 
    const string& filesystem_root) 
{
    string decoded = decode_url(url_path);
    // logger.log("decoded = ", decoded);
    
    fs::path root_dir = fs::weakly_canonical("./" + filesystem_root);

    string url_prefix = "/" + filesystem_root;
    if (!decoded.starts_with(url_prefix) && !decoded.starts_with(url_prefix + "/"))
        throw std::runtime_error("invalid root");
    
    string rel = decoded.substr(("/" + filesystem_root).size()); // remove "/root/"
    if (!rel.empty() && rel.front() == '/')
        rel.erase(0, 1);
    fs::path relative(rel);
    // std::cout << relative.string() << "\n";

    fs::path requested = fs::weakly_canonical(root_dir / relative);
    // std::cout << requested.string() << "\n";

    if (requested.string().rfind(root_dir.string(), 0) != 0) {
        throw std::runtime_error("path traversal");
    }
    // std::cout << requested << "\n";
    return requested;   
}

string FileHandler::decode_url(const string& str) {
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

string FileHandler::generate_breadcrumbs(const string& url_path) {
    string html;

    std::istringstream ss(url_path);
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
ByteRange FileHandler::parse_range(const string& header, size_t file_size) {
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

size_t FileHandler::get_file_size(const string& file_path) {
    return  std::filesystem::file_size(file_path);
}
