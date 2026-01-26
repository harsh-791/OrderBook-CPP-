#pragma once

#include <cstdint>
#include <string>
#include <ctime>

struct Message {
    static constexpr size_t INSTRUMENT_SIZE = 32;
    char instrument[INSTRUMENT_SIZE];
    double bid;
    double ask;
    int64_t timestamp_ns;
};

inline int64_t get_timestamp_ns() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

inline std::string to_json(const Message& msg) {
    std::string json = "{";
    json += "\"instrument\":\"" + std::string(msg.instrument) + "\",";
    json += "\"bid\":" + std::to_string(msg.bid) + ",";
    json += "\"ask\":" + std::to_string(msg.ask) + ",";
    json += "\"timestamp_ns\":" + std::to_string(msg.timestamp_ns);
    json += "}";
    return json;
}
