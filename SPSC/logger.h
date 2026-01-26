#pragma once

#include "message.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

inline void log_message(const Message& msg) {
    int64_t ts_ns = msg.timestamp_ns;
    int64_t sec = ts_ns / 1000000000LL;
    int64_t nsec = ts_ns % 1000000000LL;
    
    time_t t = sec;
    struct tm* tm_info = localtime(&t);
    
    std::ostringstream oss;
    oss << "["
        << std::setfill('0') << std::setw(2) << tm_info->tm_hour << ":"
        << std::setfill('0') << std::setw(2) << tm_info->tm_min << ":"
        << std::setfill('0') << std::setw(2) << tm_info->tm_sec << "."
        << std::setfill('0') << std::setw(9) << nsec << "] "
        << msg.instrument
        << " BID=" << std::fixed << std::setprecision(2) << msg.bid
        << " ASK=" << std::fixed << std::setprecision(2) << msg.ask;
    
    std::cout << oss.str() << "\n";
    std::cout.flush();
}
