#include "ring_buffer.h"
#include "logger.h"
#include <unistd.h>

int main() {
    const char* shm_name = "/market_data_ring";
    
    RingBufferReader reader(shm_name);
    if (!reader.is_valid()) {
        std::cerr << "Failed to create ring buffer reader\n";
        return 1;
    }
    
    Message msg;
    
    while (true) {
        if (reader.pop(msg)) {
            log_message(msg);
        } else {
            usleep(100);
        }
    }
    
    return 0;
}
