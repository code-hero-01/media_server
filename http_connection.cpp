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
            logger.log(string("Client ("), client_ip, ") closed connection");             
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

void HttpConnection::send_response(const string& msg) {
    size_t total = 0;
    while (total < msg.size()) {
        ssize_t bytes_sent =
            send(client_fd,
                msg.c_str() + total,
                msg.size() - total,
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

    logger.log("sent message to client (", client_ip, ")");
}
