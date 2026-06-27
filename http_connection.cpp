#include "header_files/http_connection.h"

HttpConnection::HttpConnection(int client_fd, const string& client_ip) 
    : client_fd(client_fd), client_ip(client_ip) {}

std::optional<Request> HttpConnection::read_request() {     
    std::string request_msg;     
    char buffer[1024];     
    size_t header_end;
    while (true) {         
        int bytes = recv(client_fd, buffer, sizeof(buffer), 0);         
        if (bytes == -1) {             
            logger.log("recv: ", std::strerror(errno));             
            close(client_fd);             
            return std::nullopt;
        }         
        if (bytes == 0) {             
            logger.log(string("client ("), client_ip, ") closed connection");             
            close(client_fd);             
            return std::nullopt; 
        }         
        request_msg.append(buffer, bytes);         
        if ((header_end = request_msg.find("\r\n\r\n")) != std::string::npos) 
            break;
    }      
    
    Request req(request_msg.substr(0, header_end + 4));
    size_t body_start = header_end + 4;
    // std::cout << request_msg << "\n";
    if (body_start < request_msg.size()) { // part of body already receievd
        req.body = request_msg.substr(body_start);
    }
    
    if (req.has_header("Content-Length")) {
        int content_length = stoi(req.get_header("Content-Length"));
        while (req.body.size() < content_length) {
            int bytes = recv(client_fd, buffer, sizeof(buffer), 0);         
            if (bytes == -1) {             
                logger.log("recv: ", std::strerror(errno));                          
                return std::nullopt;
            }         
            if (bytes == 0) {             
                logger.log(string("Client ("), client_ip, ") closed connection");                          
                return std::nullopt; 
            }         
            
            req.body.append(buffer, bytes);
        }
    }

    return req; 
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
    char buffer[16384];  // send 16KB at a time from start to end
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
