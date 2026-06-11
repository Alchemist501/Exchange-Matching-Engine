#pragma once

#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <mutex>

namespace Logger {
    inline std::mutex& getMutex() {
        static std::mutex mtx;
        return mtx;
    }

    inline void log(const std::string& level, const std::string& color, const std::string& message) {
        std::lock_guard<std::mutex> lock(getMutex());
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S") 
           << '.' << std::setfill('0') << std::setw(3) << ms.count();

        std::cout << "\033[90m[" << ss.str() << "]\033[0m "
                  << color << "[" << level << "]\033[0m "
                  << message << "\n";
    }

    inline void info(const std::string& message) {
        log("INFO", "\033[32m", message); // Green
    }

    inline void warn(const std::string& message) {
        log("WARN", "\033[33m", message); // Yellow
    }

    inline void error(const std::string& message) {
        log("ERROR", "\033[31m", message); // Red
    }

    inline void debug(const std::string& message) {
#ifndef NDEBUG
        log("DEBUG", "\033[36m", message); // Cyan
#endif
    }
}
