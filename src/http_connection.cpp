#include "http_connection.h"

HttpConnection::HttpConnection(
    int client_fd, 
    const string& client_ip, 
    const string& root) 
    : client_fd(client_fd), client_ip(client_ip), ROOT(root) {}

std::optional<Request> HttpConnection::read_request() {
    size_t crlf_pos;
    char buffer[1024]; 
    
    // read headers
    while ((crlf_pos = recv_buffer.find("\r\n\r\n")) == string::npos) {
        if (!receieve(buffer, 1024)) return std::nullopt; 
    }
    size_t header_end = crlf_pos + 4;
    Request req(recv_buffer.substr(0, header_end));
    recv_buffer.erase(0, header_end);
    
    if (req.has_header("Content-Type") 
        && req.get_header("Content-Type").find("multipart/form-data") != string::npos) 
    {
        return req;
    } 
    
    // read body
    if (req.has_header("Content-Length")) {
        int content_length = stoi(req.get_header("Content-Length"));
        while (recv_buffer.size() < content_length) {
            if (!receieve(buffer, 1024)) break;
        }

        req.body.append(recv_buffer.substr(0, content_length));
        recv_buffer.erase(0, content_length);
    }

    return req;
}

bool HttpConnection::receieve(char* buffer, size_t buffer_size) {
    int bytes = recv(client_fd, buffer, buffer_size, 0);         
    if (bytes == -1) {             
        logger.log("recv: ", std::strerror(errno));                          
        return false;
    }         
    if (bytes == 0) {             
        logger.log(string("client ("), client_ip, ") closed connection");                          
        return false; 
    }
    
    recv_buffer.append(buffer, bytes);
    return true;
}


void HttpConnection::send_response(Response& res) {
    send_text(res.serialize());
    if (res.is_file)
        send_file(res);
}

void HttpConnection::send_text(const string& text) {
    //std::cout << text << "\n\n";
    size_t total = 0;
    while (total < text.size()) {
        ssize_t bytes_sent =
            send(client_fd,
                text.c_str() + total,
                text.size() - total,
                0);

        if (bytes_sent == -1) {
            if (errno == EPIPE || errno == ECONNRESET) {
                logger.log("client (", client_ip, ") disconnected");
            } else {
                logger.log("send: ", std::strerror(errno));
            }
            return;
        }

        total += bytes_sent;
    }

    //std::cout << headers << "\n\n";
    logger.log("sent message to client (", client_ip, ")");
}

void HttpConnection::send_file(Response& res) {
    std::ifstream file(res.path, std::ios::binary);
    if (!file.is_open()) {
        logger.log("Error: Could not open the file: ", res.path);
        Response error_res(400, "text/html", "<h1>404 Not Found<h1>");
        send_response(error_res);
        return;
    }

    size_t file_size = file_handler::get_file_size(res.path);

    if (res.end == SIZE_MAX)
        res.end = file_size - 1;

    if (res.start > res.end || res.end >= file_size)
        return;
   
    file.seekg(res.start);
    char buffer[16 * 1024];  // send 16KB at a time from start to end
    size_t remaining = res.end - res.start + 1;
    
    size_t sent = 0;
    while (remaining > 0) {
        size_t chunk = std::min(remaining, sizeof(buffer));
        file.read(buffer, chunk);
        if (file.gcount() <= 0) break;
        
        if (!send_all(buffer, file.gcount())) 
            return;
        sent += file.gcount();
        remaining -= file.gcount();
    }

    logger.log("sent file \"", res.path, "\" to client (", client_ip, ") ");
    // logger.log("expected ", res.end - res.start + 1, " sent ", sent);
}

bool HttpConnection::send_all(char* buffer, size_t chunk) {
    size_t total = 0;
    while (total < chunk) {
        ssize_t bytes_sent =
            send(client_fd,
                buffer + total,
                chunk - total,
                0);

        if (bytes_sent == -1) {
            if (errno == EPIPE || errno == ECONNRESET) {
                // logger.log("client (", client_ip, ") disconnected");
            } else {
                logger.log("send: ", std::strerror(errno));
            }
            return false;
        }

        total += bytes_sent;
    }
    
    // logger.log("sent message to client (", client_ip, ") ", chunk);
    return true;
}


MultipartParser::MultipartParser(HttpConnection& conn) 
    : conn(conn) {}

bool MultipartParser::set_boundary(const Request& req) {
    size_t pos;
    
    if (!req.has_header("Content-Type"))
        return false;

    string content_type_header = req.get_header("Content-Type");
    if ((pos = content_type_header.find("boundary=")) == string::npos) 
        return false;
    
    boundary = content_type_header.substr(pos + 9);
    return true;
} 

bool MultipartParser::read_subheaders() {
    char buffer[1024];
    size_t pos;
    while ((pos = conn.recv_buffer.find("\r\n\r\n")) == string::npos) {
        if (!conn.receieve(buffer, 1024)) 
            return false;
    } 

    auto start_pos = conn.recv_buffer.find("filename=\"");
    if (start_pos == string::npos) return false;
    start_pos += 10; // skip filename="
    
    auto end_pos = conn.recv_buffer.find("\"", start_pos);
    if (end_pos == std::string::npos) return false;

    filename = conn.recv_buffer.substr(start_pos, end_pos - start_pos);
    conn.recv_buffer.erase(0, pos + 4);
    return true;
}


bool MultipartParser::stream_to(const string& path) {
    std::ofstream file(path, std::ios::binary);

    if (!file.is_open()) {
        logger.log("Error: Could not open the file: ", path);
        return false;
    }

    char buffer[256 * 1024];
    string end_boundary = "\r\n--" + boundary + "--\r\n";
    while (true) {
        size_t pos;
        if ((pos = conn.recv_buffer.find(end_boundary)) != string::npos) {
            file.write(conn.recv_buffer.data(), pos);
            conn.recv_buffer.erase(0, pos + end_boundary.size());
            file.close();
            return true;
        }
        else if (conn.recv_buffer.size() > end_boundary.size()) {
            size_t safe = conn.recv_buffer.size() - end_boundary.size();
            file.write(conn.recv_buffer.data(), safe);
            conn.recv_buffer.erase(0, safe);
        }
        
        if (!conn.receieve(buffer, sizeof(buffer))) 
            return false;
    }
}
