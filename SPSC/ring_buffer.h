#pragma once

#include "message.h"
#include <atomic>
#include <cstring>
#include <new>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

static constexpr size_t BUFFER_SIZE = 1024;

struct RingBuffer {
    alignas(64) std::atomic<size_t> write_idx;
    alignas(64) std::atomic<size_t> read_idx;
    Message buffer[BUFFER_SIZE];
    
    RingBuffer() : write_idx(0), read_idx(0) {}
};

class RingBufferWriter {
public:
    RingBufferWriter(const char* shm_name) {
        int fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
        if (fd == -1) {
            return;
        }
        
        size_t size = sizeof(RingBuffer);
        bool created = (ftruncate(fd, size) == 0);
        
        rb = static_cast<RingBuffer*>(mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
        close(fd);
        
        if (rb == MAP_FAILED) {
            rb = nullptr;
            return;
        }
        
        if (created) {
            new(rb) RingBuffer();
        }
    }
    
    ~RingBufferWriter() {
        if (rb && rb != MAP_FAILED) {
            munmap(rb, sizeof(RingBuffer));
        }
    }
    
    bool push(const Message& msg) {
        if (!rb) return false;
        
        size_t write_pos = rb->write_idx.load(std::memory_order_acquire);
        size_t next_write = (write_pos + 1) & (BUFFER_SIZE - 1);
        size_t read_pos = rb->read_idx.load(std::memory_order_acquire);
        
        if (next_write == read_pos) {
            return false;
        }
        
        std::memcpy(&rb->buffer[write_pos], &msg, sizeof(Message));
        rb->write_idx.store(next_write, std::memory_order_release);
        return true;
    }
    
    bool is_valid() const { return rb != nullptr && rb != MAP_FAILED; }
    
private:
    RingBuffer* rb = nullptr;
};

class RingBufferReader {
public:
    RingBufferReader(const char* shm_name) {
        int fd = shm_open(shm_name, O_RDWR, 0666);
        if (fd == -1) {
            return;
        }
        
        size_t size = sizeof(RingBuffer);
        rb = static_cast<RingBuffer*>(mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
        close(fd);
        
        if (rb == MAP_FAILED) {
            rb = nullptr;
        }
    }
    
    ~RingBufferReader() {
        if (rb && rb != MAP_FAILED) {
            munmap(rb, sizeof(RingBuffer));
        }
    }
    
    bool pop(Message& msg) {
        if (!rb) return false;
        
        size_t read_pos = rb->read_idx.load(std::memory_order_acquire);
        size_t write_pos = rb->write_idx.load(std::memory_order_acquire);
        
        if (read_pos == write_pos) {
            return false;
        }
        
        std::memcpy(&msg, &rb->buffer[read_pos], sizeof(Message));
        size_t next_read = (read_pos + 1) & (BUFFER_SIZE - 1);
        rb->read_idx.store(next_read, std::memory_order_release);
        return true;
    }
    
    bool is_valid() const { return rb != nullptr && rb != MAP_FAILED; }
    
private:
    RingBuffer* rb = nullptr;
};
