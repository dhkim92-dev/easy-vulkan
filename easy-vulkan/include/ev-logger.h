#pragma once

#include <cstdio>
#include <cstdlib>
#include <string>
#include <mutex>

namespace ev::log {

    enum class LogLevel {
        DEBUG = 0,
        INFO,
        WARNING,
        ERROR,
    };

    #ifndef EV_LOG_LEVEL
        #define EV_LOG_LEVEL 1
    #endif

    constexpr bool is_level_enabled(LogLevel level) {
        return static_cast<int>(level) >= EV_LOG_LEVEL;
    }

    constexpr const char* level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }

    #define ev_log_debug(fmt, ...) \
        do { \
            if constexpr (ev::log::is_level_enabled(ev::log::LogLevel::DEBUG)) { \
                fprintf(stderr, "[DEBUG][%s:%d] : ", __FILE__, __LINE__); \
                fprintf(stderr, fmt __VA_OPT__(,) __VA_ARGS__); \
                fprintf(stderr, "\n"); \
                fflush(stderr); \
            } \
        } while(0)

    #define ev_log_info(fmt, ...) \
        do { \
            if constexpr (ev::log::is_level_enabled(ev::log::LogLevel::INFO)) { \
                fprintf(stderr, "[INFO][%s:%d] : ", __FILE__, __LINE__); \
                fprintf(stderr, fmt __VA_OPT__(,) __VA_ARGS__); \
                fprintf(stderr, "\n"); \
                fflush(stderr); \
            } \
        } while(0)

    #define ev_log_warn(fmt, ...) \
        do { \
            if constexpr (ev::log::is_level_enabled(ev::log::LogLevel::WARNING)) { \
                fprintf(stderr, "[WARNING][%s:%d] : ", __FILE__, __LINE__); \
                fprintf(stderr, fmt __VA_OPT__(,) __VA_ARGS__); \
                fprintf(stderr, "\n"); \
                fflush(stderr); \
            } \
        } while(0)

    #define ev_log_error(fmt, ...) \
        do { \
            if constexpr (ev::log::is_level_enabled(ev::log::LogLevel::ERROR)) { \
                fprintf(stderr, "[ERROR][%s:%d] : ", __FILE__, __LINE__); \
                fprintf(stderr, fmt __VA_OPT__(,) __VA_ARGS__); \
                fprintf(stderr, "\n"); \
                fflush(stderr); \
            } \
        } while(0)
}
