#include "header_files/router.h"

Router::Router(const string& root_dir) :
    ROOT(root_dir) {}

Response Router::route(const Request& req, HttpConnection& conn)
{
    if (req.version != "HTTP/1.1") {
        return Response(
            505,
            "text/html",
            "<h1>505 HTTP Version Not Supported</h1>"
        );
    }

    else if (req.method == "GET") { 
        // homepage
        if (req.path == "/") {
            return handle_home(req);
        }

        // static file request
        else if (req.path.starts_with("/static/")) {
            return handle_media(req, "static");
        }

        // file or folder request
        else if (req.path.starts_with("/" + ROOT) || req.path.starts_with("/" + ROOT + "/")) {
            return handle_media(req, ROOT);
        }

        // template routes (/about, /contact, etc.)
        else { 
            return handle_template(req);
        }
    }

    if (req.method == "POST") {
        if (req.path.ends_with("/upload")) {
            return handle_upload(req, conn);
        }
        else if (req.path.ends_with("/mkdir")) {
            return handle_mkdir(req);
        }
        else if (req.path.ends_with("/delete")) {
            return handle_delete(req);
        }
        else if (req.path.ends_with("/rename")) {
            return handle_rename(req);
        }
        else if (req.path.ends_with("/download")) {
            return handle_media(req, ROOT);
        }
        else 
            return Response(405, "text/html", "<h1>405 Method Not Allowed</h1>");
    }

    else {
        return Response(
                405,
                "text/html",
                "<h1>405 Method Not Allowed</h1>"
            );
    }
}

Response Router::handle_home(const Request& req) {
    string page;
        
    if (!file_handler::read_template("templates/home.html", page)) 
        return Response(404, "text/html", "<h1>404 Not Found</h1>");
    
    string content_list = file_handler::generate_dir_listing("./" + ROOT, ROOT);
    file_handler::render_template(page, "{{ROOT}}", "./" + ROOT);
    file_handler::render_template(page, "{{CURR_DIR}}", "./" + ROOT);    
    file_handler::render_template(page, "{{CONTENT_LIST}}", content_list);
    
    return Response(200, "text/html", page);
}

Response Router::handle_template(const Request& req) { 
    string page;
    string path = "/templates" + req.path + ".html";
    try {
        //logger.log("req.path = ", req.path);
        path = file_handler::resolve_path(path, "templates");
    } catch (const std::exception& e) {
        std::cerr << "file resolution error: " << e.what() << "\n";
        return Response(403, "text/html", "<h1>403 Forbidden</h1>");
    }

    if (!file_handler::read_template(path, page))
        return Response(404, "text/html", "<h1>404 Not Found</h1>");

    return Response(200, "text/html", page);
}


Response Router::handle_media(const Request& req, const string& fs_root) {
    string path;
    try {
        //logger.log("req.path = ", req.path);
        path = file_handler::resolve_path(req.path, fs_root);
    } catch (const std::exception& e) {
        std::cerr << "file resolution error: " << e.what() << "\n";
        return Response(403, "text/html", "<h1>403 Forbidden</h1>");
    }
    
    bool download = false;
    if (path.ends_with("/download")) {
        download = true;
        path = path.substr(0, path.size() - 9);
    }

    if (!fs::exists(path)) {
        logger.log("\"", path, "\" does not exit\n");
        return Response(400, "text/html", "<h1>404 Not Found<h1>");
    } 
    string filename = fs::path(path).filename().string();

    if (std::filesystem::is_directory(path)) {  // if directory is requested
        string page;
        if (!file_handler::read_template("templates/subdir.html", page)) 
            return Response(404, "text/html", "<h1>404 Not Found</h1>");
        
        string breadcrumbs = file_handler::generate_breadcrumbs(decode_url(req.path));
        file_handler::render_template(page, "{{BREADCRUMBS}}", breadcrumbs);
        
        file_handler::render_template(page, "{{CURR_DIR}}", req.path);

        string content_list = file_handler::generate_dir_listing(path, ROOT);
        file_handler::render_template(page, "{{CONTENT_LIST}}", content_list);
        
        Response res(200, "text/html", page);
        // if (download == true) { 
        //     res.headers["Content-Type"] = "application/zip";   
        //     res.headers["Content-Disposition"] = "attachment; filename=\"" + filename + "\"";
        // }
        return res;
    }    
    else {  // if file is requested
        string content;
        if (req.has_header("Range")) { // partial response
            size_t file_size = file_handler::get_file_size(path);
            ByteRange range = file_handler::parse_range(req.get_header("Range"), file_size);
            
            if (!range.valid)  
                return Response(400, "text/html", "<h1>400 Bad Request</h1>");
            
            Response res(206, file_handler::get_content_type(path), content);

            res.headers["Accept-Ranges"] = "bytes";
            res.headers["Content-Range"] =
                "bytes "
                + std::to_string(range.start)
                + "-"
                + std::to_string(range.end)
                + "/"
                + std::to_string(file_size);

            res.is_file = true;
            res.start = range.start;
            res.end = range.end;
            res.path = path;    
            res.content_length = range.end - range.start + 1;

            if (download == true) 
                res.headers["Content-Disposition"] = "attachment; filename=\"" + filename + "\"";

            return res;
        }   
        else {
            // if (!file_handler::read_file(path, content)) 
            //     return Response(404, "text/html", "<h1>404 Not Found</h1>");

            Response res(200, file_handler::get_content_type(path), content);
            res.is_file = true;
            res.path = path;
            res.content_length = file_handler::get_file_size(path);

            if (download == true) 
                res.headers["Content-Disposition"] = "attachment; filename=\"" + filename + "\"";

            return res;
        }
    }
}

Response Router::handle_upload(const Request& req, HttpConnection& conn) {
    MultipartParser parser(conn);
    if (!parser.set_boundary(req) || !parser.read_subheaders()) {
        logger.log("bad upload request");
        return Response(400, "text/html", "<h1>400 Bad Request</h1>");
    }
    
    string url_dir = req.path.substr(0, req.path.size() - 7); // remove "/upload"
    string dest_dir;
    try {
        dest_dir = file_handler::resolve_path(url_dir, ROOT);
    } catch (const std::exception& e) {
        std::cerr << "file resolution error: " << e.what() << "\n";
        return Response(403, "text/html", "<h1>403 Forbidden</h1>");
    }
    
    string filename = fs::path(parser.filename).filename().string();
    string path = dest_dir + '/' + filename;
    logger.log("Downloading '", filename, "' to '", dest_dir, "'...");

    if (!parser.stream_to(path)) {
        logger.log("failed to download '", filename, "'");
        return Response(500, "text/html", "<h1>500 Internal Server Error</h1>");
    }
    logger.log("'", filename, "' downloaded successfully to '", dest_dir, "'");
    Response res(303, "text/plain", "");
    res.headers["Location"] = url_dir;
    return res;    
}

Response Router::handle_mkdir(const Request& req) {
    string url_dir = req.path.substr(0, req.path.size() - 6); // remove "/mkdir"
    string dest_dir;
    try {
        dest_dir = file_handler::resolve_path(url_dir, ROOT);
    } catch (const std::exception& e) {
        std::cerr << "file resolution error: " << e.what() << "\n";
        return Response(403, "text/html", "<h1>403 Forbidden</h1>");
    }
    
    UrlEncodedForm form(req.body);
    string dir_name;
    if (form.has_field("folder_name")) 
        dir_name = form.get_field("folder_name");
    else
        return Response(400, "text/html", "<h1>400 Bad Request</h1>");

    string path = dest_dir + '/' + dir_name;

    if (!file_handler::mkdir(path)) {
        logger.log("failed to create directory '", dir_name, "'");
        return Response(500, "text/html", "<h1>500 Internal Server Error</h1>");
    }
    logger.log("'", dir_name, "' directory creatory succesffuly in '", dest_dir, "'");
    Response res(303, "text/plain", "");
    res.headers["Location"] = url_dir;
    return res;
}

Response Router::handle_delete(const Request& req) {
    string url_path = req.path.substr(0, req.path.size() - 7); // remove "/delete"
    string path;
    try {
        path = file_handler::resolve_path(url_path, ROOT);
    } catch (const std::exception& e) {
        std::cerr << "file resolution error: " << e.what() << "\n";
        return Response(403, "text/html", "<h1>403 Forbidden</h1>");
    }

    if (!file_handler::delete_file(path))  {
        logger.log("failed to delete: \"", path, "\"");
        return Response(500, "text/html", "<h1>500 Internal Server Error</h1>");
    }
    logger.log("\"", path, "\" successfully deleted");
    Response res(303, "text/plain", "");
    
    fs::path parent_url = fs::path(url_path).parent_path();
    res.headers["Location"] = parent_url.string();
    return res;
}

Response Router::handle_rename(const Request& req) {
    string url_path = req.path.substr(0, req.path.size() - 7); // remove "/rename"
    string old_path;
    try {
        old_path = file_handler::resolve_path(url_path, ROOT);
    } catch (const std::exception& e) {
        std::cerr << "file resolution error: " << e.what() << "\n";
        return Response(403, "text/html", "<h1>403 Forbidden</h1>");
    }

    fs::path parent_url = fs::path(url_path).parent_path();

    UrlEncodedForm form(req.body);
    string new_name;
    if (form.has_field("new_name")) 
        new_name = form.get_field("new_name");
    else
        return Response(400, "text/html", "<h1>400 Bad Request</h1>");

    string new_path = parent_url.string() + "/" + new_name;
    std::cout << new_path;
    try {
        new_path = file_handler::resolve_path(new_path, ROOT);
    } catch (const std::exception& e) {
        std::cerr << "file resolution error: " << e.what() << "\n";
        return Response(403, "text/html", "<h1>403 Forbidden</h1>");
    }

    if (!file_handler::rename_file(old_path, new_path)) {
        logger.log("failed to rename: \"", old_path, "\" to \"", new_name, "\"");
        return Response(500, "text/html", "<h1>500 Internal Server Error</h1>");
    }
    logger.log("\"", old_path, "\" successfully renamed to \"", new_name, "\"");
    Response res(303, "text/plain", "");
    
    res.headers["Location"] = parent_url.string();
    return res;
}

