#include "message.h"
#include "logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <string>

static constexpr int PORT = 8888;

bool parse_json(const std::string& json, Message& msg) {
    size_t inst_pos = json.find("\"instrument\":\"");
    if (inst_pos == std::string::npos) return false;
    inst_pos += 14;
    size_t inst_end = json.find("\"", inst_pos);
    if (inst_end == std::string::npos) return false;
    std::string inst = json.substr(inst_pos, inst_end - inst_pos);
    std::strncpy(msg.instrument, inst.c_str(), Message::INSTRUMENT_SIZE - 1);
    msg.instrument[Message::INSTRUMENT_SIZE - 1] = '\0';
    
    size_t bid_pos = json.find("\"bid\":");
    if (bid_pos == std::string::npos) return false;
    bid_pos += 6;
    msg.bid = std::stod(json.substr(bid_pos));
    
    size_t ask_pos = json.find("\"ask\":");
    if (ask_pos == std::string::npos) return false;
    ask_pos += 6;
    msg.ask = std::stod(json.substr(ask_pos));
    
    size_t ts_pos = json.find("\"timestamp_ns\":");
    if (ts_pos == std::string::npos) return false;
    ts_pos += 15;
    msg.timestamp_ns = std::stoll(json.substr(ts_pos));
    
    return true;
}


int connect_to_server() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }
    
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(PORT);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    
    fcntl(sock, F_SETFL, O_NONBLOCK);
    
    return sock;
}

int main() {
    int sock = -1;
    std::string buffer;
    
    while (true) {
        if (sock < 0) {
            sock = connect_to_server();
            if (sock < 0) {
                usleep(1000);
                continue;
            }
            buffer.clear();
        }
        
        char recv_buf[4096];
        ssize_t received = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
        
        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(100);
                continue;
            }
            close(sock);
            sock = -1;
            continue;
        }
        
        if (received == 0) {
            close(sock);
            sock = -1;
            continue;
        }
        
        recv_buf[received] = '\0';
        buffer += recv_buf;
        
        size_t pos = 0;
        while (true) {
            size_t newline = buffer.find('\n', pos);
            if (newline == std::string::npos) {
                buffer = buffer.substr(pos);
                break;
            }
            
            std::string line = buffer.substr(pos, newline - pos);
            pos = newline + 1;
            
            Message msg;
            if (parse_json(line, msg)) {
                log_message(msg);
            }
        }
    }
    
    if (sock >= 0) {
        close(sock);
    }
    
    return 0;
}
