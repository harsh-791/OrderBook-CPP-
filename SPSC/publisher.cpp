#include "ring_buffer.h"
#include "message.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <random>
#include <string>

static constexpr int PORT = 8888;
static constexpr int SEND_BUFFER_SIZE = 4096;
static constexpr const char* SHM_NAME = "/market_data_ring";

int create_server_socket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }
    
    int flag = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    
    int send_buf = SEND_BUFFER_SIZE;
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &send_buf, sizeof(send_buf));
    
    fcntl(sock, F_SETFL, O_NONBLOCK);
    
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(PORT);
    
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    
    if (listen(sock, 1) < 0) {
        close(sock);
        return -1;
    }
    
    return sock;
}

int accept_client(int server_sock) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
    
    if (client < 0) {
        return -1;
    }
    
    int flag = 1;
    setsockopt(client, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    
    fcntl(client, F_SETFL, O_NONBLOCK);
    
    return client;
}

int main() {
    int server_sock = create_server_socket();
    if (server_sock < 0) {
        return 1;
    }
    
    RingBufferWriter shm_writer(SHM_NAME);
    if (!shm_writer.is_valid()) {
        close(server_sock);
        return 1;
    }
    
    int client_sock = -1;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> price_variation(-0.5, 0.5);
    
    double base_bid = 2850.0;
    
    Message msg;
    std::strcpy(msg.instrument, "RELIANCE");
    
    while (true) {
        if (client_sock < 0) {
            client_sock = accept_client(server_sock);
        }
        
        msg.bid = base_bid + price_variation(gen);
        msg.ask = msg.bid + 0.5 + price_variation(gen) * 0.1;
        msg.timestamp_ns = get_timestamp_ns();
        
        shm_writer.push(msg);
        
        if (client_sock >= 0) {
            std::string json = to_json(msg);
            json += "\n";
            
            ssize_t sent = send(client_sock, json.c_str(), json.size(), MSG_NOSIGNAL);
            if (sent < 0) {
                close(client_sock);
                client_sock = -1;
            }
        }
        
        usleep(1000);
    }
    
    close(server_sock);
    if (client_sock >= 0) {
        close(client_sock);
    }
    
    return 0;
}
