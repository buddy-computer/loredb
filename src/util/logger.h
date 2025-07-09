/// \file logger.h
/// \brief Logging utilities and macros for the graph database.
/// \author wiki-graph contributors
/// \ingroup util
#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <fmt/format.h>
#include <memory>
#include <string>

namespace graphdb::util {

/**
 * @class Logger
 * @brief Static logging wrapper using spdlog for structured and level-based logging.
 *
 * Provides initialization, log level control, and structured logging helpers for operations and performance.
 */
class Logger {
public:
    /**
     * @brief Initialize the logger with console and file outputs.
     * @param log_level Log level (e.g., "info", "debug").
     * @param log_file Log file path.
     */
    static void init(const std::string& log_level = "info", 
                     const std::string& log_file = "graphdb.log");
    /**
     * @brief Get the main logger instance.
     * @return Shared pointer to spdlog logger.
     */
    static std::shared_ptr<spdlog::logger> get();
    
    // Convenience functions for different log levels
    template<typename... Args>
    static void trace(fmt::format_string<Args...> msg, Args&&... args) {
        get()->trace(msg, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void debug(fmt::format_string<Args...> msg, Args&&... args) {
        get()->debug(msg, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void info(fmt::format_string<Args...> msg, Args&&... args) {
        get()->info(msg, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void warn(fmt::format_string<Args...> msg, Args&&... args) {
        get()->warn(msg, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void error(fmt::format_string<Args...> msg, Args&&... args) {
        get()->error(msg, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void critical(fmt::format_string<Args...> msg, Args&&... args) {
        get()->critical(msg, std::forward<Args>(args)...);
    }
    
    // Structured logging helpers
    template<typename... Args>
    static void log_operation(
        const std::string& operation,
        const std::string& component,
        fmt::format_string<Args...> details,
        Args&&... args) {
        get()->info(
            "[{}] {} - {}",
            component,
            operation,
            fmt::format(details, std::forward<Args>(args)...)
        );
    }
    template<typename... Args>
    static void log_performance(
        const std::string& operation,
        double duration_ms,
        fmt::format_string<Args...> details,
        Args&&... args) {
        get()->info(
            "[PERF] {} took {:.3f}ms - {}",
            operation,
            duration_ms,
            fmt::format(details, std::forward<Args>(args)...)
        );
    }
    template<typename... Args>
    static void log_error(
        const std::string& component,
        const std::string& error_type,
        fmt::format_string<Args...> details,
        Args&&... args) {
        get()->error(
            "[{}] {} - {}",
            component,
            error_type,
            fmt::format(details, std::forward<Args>(args)...)
        );
    }

private:
    static std::shared_ptr<spdlog::logger> logger_;
};

// Convenience macros for common logging patterns
#define LOG_TRACE(...) graphdb::util::Logger::trace(__VA_ARGS__)
#define LOG_DEBUG(...) graphdb::util::Logger::debug(__VA_ARGS__)
#define LOG_INFO(...) graphdb::util::Logger::info(__VA_ARGS__)
#define LOG_WARN(...) graphdb::util::Logger::warn(__VA_ARGS__)
#define LOG_ERROR(...) graphdb::util::Logger::error(__VA_ARGS__)
#define LOG_CRITICAL(...) graphdb::util::Logger::critical(__VA_ARGS__)

#define LOG_OPERATION(component, operation, ...) \
    graphdb::util::Logger::log_operation(operation, component, __VA_ARGS__)

#define LOG_PERFORMANCE(operation, duration_ms, ...) \
    graphdb::util::Logger::log_performance(operation, duration_ms, __VA_ARGS__)

#define LOG_ERROR_DETAILED(component, error_type, ...) \
    graphdb::util::Logger::log_error(component, error_type, __VA_ARGS__)

}  // namespace graphdb::util