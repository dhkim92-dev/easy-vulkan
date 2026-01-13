#pragma once

#include <cstdio>
#include <mutex>

namespace ev::log {

    enum class LogLevel {
        DEBUG = 0,
        INFO,
        WARNING,
        ERROR,
    };

    // ===== 컴파일 타임 로그 레벨 =====
    // 0=DEBUG, 1=INFO, 2=WARNING, 3=ERROR
    #ifndef EV_LOG_LEVEL
        #define EV_LOG_LEVEL 1
    #endif

    constexpr int to_int(LogLevel lvl) {
        return static_cast<int>(lvl);
    }

    inline std::mutex& log_mutex() {
        static std::mutex m;
        return m;
    }

    inline const char* level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:   return "DEBUG";
            case LogLevel::INFO:    return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR:   return "ERROR";
            default:               return "UNKNOWN";
        }
    }

    inline void log_impl_noop() {}

} // namespace ev::log

// ================== 매크로 ==================

#define EV_LOG_IMPL(level_enum, fmt, ...)                                     \
    do {                                                                      \
        if constexpr (EV_LOG_LEVEL <= ev::log::to_int(level_enum)) {         \
            std::lock_guard<std::mutex> __ev_log_lock(ev::log::log_mutex()); \
            std::fprintf(stderr, "[%s][%s:%d] : ", ev::log::level_to_string(level_enum), __FILE__, __LINE__); \
            std::fprintf(stderr, fmt, ##__VA_ARGS__);                        \
            std::fprintf(stderr, "\n");                                    \
            std::fflush(stderr);                                              \
        }                                                                     \
    } while (0)

#define ev_log_debug(fmt, ...)   EV_LOG_IMPL(ev::log::LogLevel::DEBUG,   fmt, ##__VA_ARGS__)
#define ev_log_info(fmt, ...)    EV_LOG_IMPL(ev::log::LogLevel::INFO,    fmt, ##__VA_ARGS__)
#define ev_log_warn(fmt, ...)    EV_LOG_IMPL(ev::log::LogLevel::WARNING, fmt, ##__VA_ARGS__)
#define ev_log_error(fmt, ...)   EV_LOG_IMPL(ev::log::LogLevel::ERROR,   fmt, ##__VA_ARGS__)
