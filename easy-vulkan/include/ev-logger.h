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

    // 실제 출력 구현 (여기만 함수 스택 탐)
    template <typename... Args>
    inline void log_impl(LogLevel level, const char* file, int line, const char* fmt, Args&&... args) {
        std::lock_guard<std::mutex> lock(log_mutex());
        std::fprintf(stderr, "[%s][%s:%d] : ", level_to_string(level), file, line);
        std::fprintf(stderr, fmt, std::forward<Args>(args)...);
        std::fprintf(stderr, "\n");
        std::fflush(stderr);
    }

} // namespace ev::log

// ================== 매크로 ==================

#define EV_LOG_IMPL(level_enum, fmt, ...)                                     \
    do {                                                                      \
        if constexpr (EV_LOG_LEVEL <= ev::log::to_int(level_enum)) {         \
            ev::log::log_impl(level_enum, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
        }                                                                     \
    } while (0)

#define ev_log_debug(fmt, ...)   EV_LOG_IMPL(ev::log::LogLevel::DEBUG,   fmt, ##__VA_ARGS__)
#define ev_log_info(fmt, ...)    EV_LOG_IMPL(ev::log::LogLevel::INFO,    fmt, ##__VA_ARGS__)
#define ev_log_warn(fmt, ...)    EV_LOG_IMPL(ev::log::LogLevel::WARNING, fmt, ##__VA_ARGS__)
#define ev_log_error(fmt, ...)   EV_LOG_IMPL(ev::log::LogLevel::ERROR,   fmt, ##__VA_ARGS__)
