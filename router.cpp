#include "header_files/router.h"

Router::Router(const string& root_dir) :
    ROOT(root_dir) {}

Response Router::route(const Request& req)
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
            return handle_upload(req);
        }
        if (req.path.ends_with("/mkdir")) {
            return handle_mkdir(req);
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
        
    if (!file_handler.serve_template("templates/home.html", page)) 
        return Response(404, "text/html", "<h1>404 Not Found</h1>");
    
    string content_list = file_handler.generate_dir_listing("./" + ROOT);
    file_handler.render_template(page, "{{ROOT}}", "./" + ROOT);
    file_handler.render_template(page, "{{CURR_DIR}}", "./" + ROOT);    
    file_handler.render_template(page, "{{CONTENT_LIST}}", content_list);
    
    return Response(200, "text/html", page);
}

Response Router::handle_template(const Request& req) { 
    string page;
    string path = "/templates" + req.path + ".html";
    try {
        //logger.log("req.path = ", req.path);
        path = file_handler.resolve_path(path, "templates");
    } catch (const std::exception& e) {
        std::cerr << "file resolution error: " << e.what() << "\n";
        return Response(403, "text/html", "<h1>403 Forbidden</h1>");
    }

    if (!file_handler.serve_template(path, page))
        return Response(404, "text/html", "<h1>404 Not Found</h1>");

    return Response(200, "text/html", page);
}

Response Router::handle_static(const Request& req) {
    string path = "./" + (req.path);
    string content;
    if (!file_handler.serve_file(path, content)) 
        return Response(404, "text/html", "<h1>404 Not Found</h1>");

    return Response(200, file_handler.get_content_type(path), content);
}

Response Router::handle_media(const Request& req, const string& fs_root) {
    string path;
    try {
        //logger.log("req.path = ", req.path);
        path = file_handler.resolve_path(req.path, fs_root);
    } catch (const std::exception& e) {
        std::cerr << "file resolution error: " << e.what() << "\n";
        return Response(403, "text/html", "<h1>403 Forbidden</h1>");
    }
    
    if (std::filesystem::is_directory(path)) {  // if directory is requested
        string page;
        if (!file_handler.serve_template("templates/subdir.html", page)) 
            return Response(404, "text/html", "<h1>404 Not Found</h1>");
        
        string breadcrumbs = file_handler.generate_breadcrumbs(req.path);
        file_handler.render_template(page, "{{BREADCRUMBS}}", breadcrumbs);
        
        file_handler.render_template(page, "{{CURR_DIR}}", req.path);

        string content_list = file_handler.generate_dir_listing(path);
        file_handler.render_template(page, "{{CONTENT_LIST}}", content_list);
        
        return Response(200, "text/html", page);
    }    
    else {  // if file is requested
        string content;
        if (req.has_header("Range")) { // partial response
            size_t file_size = file_handler.get_file_size(path);
            ByteRange range = file_handler.parse_range(req.get_header("Range"), file_size);
            
            if (!range.valid || !file_handler.serve_file(path, content, range.start, range.end)) 
                return Response(404, "text/html", "<h1>404 Not Found</h1>");
            
            Response res(206, file_handler.get_content_type(path), content);

            res.headers["Accept-Ranges"] = "bytes";
            res.headers["Content-Range"] =
                "bytes "
                + std::to_string(range.start)
                + "-"
                + std::to_string(range.end)
                + "/"
                + std::to_string(file_size);

            return res;
        }   
        else {

            if (!file_handler.serve_file(path, content)) 
                return Response(404, "text/html", "<h1>404 Not Found</h1>");

            return Response(200, file_handler.get_content_type(path), content);
        }
    }
}

Response Router::handle_upload(const Request& req) {
    FormData form(req.body);
    
    string url_dir = req.path.substr(0, req.path.size() - 7); // remove "/upload"
    string dest_dir;
    try {
        dest_dir = file_handler.resolve_path(url_dir, ROOT);
    } catch (const std::exception& e) {
        std::cerr << "file resolution error: " << e.what() << "\n";
        return Response(403, "text/html", "<h1>403 Forbidden</h1>");
    }
    
    fs::path filename = fs::path(form.filename).filename();
    string path = dest_dir + '/' + form.filename;

    if (!file_handler.download_file(path, form.data)) {
        logger.log("failed to download \"", form.filename, "\"");
        return Response(500, "text/html", "<h1>500 Internal Server Error</h1>");
    }
    logger.log("\"", form.filename, "\" downloaded successfully to \"", dest_dir, "\"");
    Response res(303, "text/plain", "");
    res.headers["Location"] = url_dir;
    return res;    
}

Response Router::handle_mkdir(const Request& req) {
    FormData form(req.body);
    
    string url_dir = req.path.substr(0, req.path.size() - 6); // remove "/mkdir"
    string dest_dir;
    try {
        dest_dir = file_handler.resolve_path(url_dir, ROOT);
    } catch (const std::exception& e) {
        std::cerr << "file resolution error: " << e.what() << "\n";
        return Response(403, "text/html", "<h1>403 Forbidden</h1>");
    }
    
    string dir_name = file_handler.decode_url(form.data);
    string path = dest_dir + '/' + dir_name;

    if (!file_handler.mkdir(path)) {
        logger.log("failed to create directory \"", dir_name, "\"");
        return Response(500, "text/html", "<h1>500 Internal Server Error</h1>");
    }
    logger.log("\"", form.data, "\" directory creatory succesffuly in \"", dest_dir, "\"");
    Response res(303, "text/plain", "");
    res.headers["Location"] = url_dir;
    return res;
}