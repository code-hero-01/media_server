#include "header_files/router.h"

Router::Router(const string& dir_name) :
    DIR_NAME(dir_name) {}

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
            return handle_static(req);
        }

        // file or folder request
        else if (req.path.starts_with("/" + DIR_NAME)) {
            return handle_media(req);
        }

        // template routes (/about, /contact, etc.)
        else { 
            return handle_template(req);
        }
    }

    if (req.method == "POST") {
        if (req.path == "/upload") {
            return handle_upload(req);
        }
        else 
            return Response(404, "text/html", "<h1>404 Not Found</h1>");
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
        
    if (!file_handler::serve_template("./home", page)) 
        return Response(404, "text/html", "<h1>404 Not Found</h1>");
    
    string content_list = file_handler::generate_content_list("./" + DIR_NAME);    
    file_handler::render_template(page, "{{CONTENT_LIST}}", content_list);
    file_handler::render_template(page, "{{DIR_NAME}}", "./" + DIR_NAME);
    
    return Response(200, "text/html", page);
}

Response Router::handle_template(const Request& req) { 
    string page;
    if (!file_handler::serve_template(req.path.substr(1), page))
        return Response(404, "text/html", "<h1>404 Not Found</h1>");

    return Response(200, "text/html", page);
}

Response Router::handle_static(const Request& req) {
    string path = file_handler::resolve_path(req.path);
    string content;
    if (!file_handler::serve_file(path, content)) 
        return Response(404, "text/html", "<h1>404 Not Found</h1>");

    return Response(200, file_handler::get_content_type(path), content);
}

Response Router::handle_media(const Request& req) {
    string path = file_handler::resolve_path(req.path);
    
    if (std::filesystem::is_directory(path)) {  // if directory is requested
        string page;
        if (!file_handler::serve_template("./subdir", page)) 
            return Response(404, "text/html", "<h1>404 Not Found</h1>");
        
        string content_list = file_handler::generate_content_list(path);
        file_handler::render_template(page, "{{CONTENT_LIST}}", content_list);
        
        string breadcrumbs = file_handler::generate_breadcrumbs(path);
        file_handler::render_template(page, "{{BREADCRUMBS}}", breadcrumbs);

        return Response(200, "text/html", page);
    }    
    else {  // if file is requested
        string content;
        if (req.has_header("Range")) { // partial response
            size_t file_size = file_handler::get_file_size(path);
            ByteRange range = file_handler::parse_range(req.get_header("Range"), file_size);
            
            if (!range.valid || !file_handler::serve_file(path, content, range.start, range.end)) 
                return Response(404, "text/html", "<h1>404 Not Found</h1>");
            
            Response res(206, file_handler::get_content_type(path), content);

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

            if (!file_handler::serve_file(path, content)) 
                return Response(404, "text/html", "<h1>404 Not Found</h1>");

            return Response(200, file_handler::get_content_type(path), content);
        }
    }
}

Response Router::handle_upload(const Request& req) {
    FormData form(req.body);
    string file_path = "./media/" + form.filename;
    if (!file_handler::download_file(file_path, form.data)) {
        return Response(500, "text/html", "<h1>500 Internal Server Error</h1>");
    }
    logger.log(file_path, " downloaded successfully");
    Response res(303, "text/plain", "");
    res.headers["Location"] = "/";
    return res;    
}