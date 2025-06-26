#pragma once

#include <cstdio>
#include <cstdlib>
#include <mutex>

namespace ev::logger {

    enum class LogLevel {
        DEBUG = 0,
        INFO,
        WARNING,
        ERROR,
    };

    // Should Be Singleton
class Logger {

private :

    Logger() = default; // Private constructor to prevent instantiation

    ~Logger() = default; // Default destructor

    Logger(const Logger&) = delete; // Prevent copy construction

    Logger& operator=(const Logger&) = delete; // Prevent assignment

    LogLevel log_level = LogLevel::DEBUG; // Set log level to DEBUG in debug mode
        // std::mutex mtx;
public:
    static Logger& getInstance() {
        static Logger* instance = new Logger(); // Create a static instance of Logger
        return *instance;
    }

    void set_log_level(LogLevel level) {
        // std::lock_guard<std::mutex> lock(mtx);
        log_level = level;
    }

    bool check_level(LogLevel level) {
        return static_cast<int>(level) >= static_cast<int>(log_level);
    }

    void info(std::string message) {
        if (check_level(LogLevel::INFO)) {
            // std::lock_guard<std::mutex> lock(mtx);
            printf("[INFO] %s\n", message.c_str());
        }
    }

    void warn(std::string message) {
        if ( check_level(LogLevel::WARNING) ) {
            // std::lock_guard<std::mutex> lock(mtx);
            printf("[WARN]: %s\n", message.c_str());
        }
    }

    void error(std::string message) {
        if (check_level(LogLevel::ERROR)) {
            // std::lock_guard<std::mutex> lock(mtx);
            printf("[ERROR]: %s\n", message.c_str());
        }
    }

    void debug(std::string message) {
        if (check_level(LogLevel::DEBUG)) {
            // std::lock_guard<std::mutex> lock(mtx);
            printf("[DEBUG] : %s\n", message.c_str());
        }
    }
};

}