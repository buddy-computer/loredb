#include "logger.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <vector>
#include <iostream>

namespace graphdb::util {

std::shared_ptr<spdlog::logger> Logger::logger_ = nullptr;

void Logger::init(const std::string& log_level, const std::string& log_file) {
    if (logger_) {
        return; // Already initialized
    }
    
    try {
        // Create sinks
        std::vector<spdlog::sink_ptr> sinks;
        
        // Console sink with colors
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info); // Console shows info and above
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        sinks.push_back(console_sink);
        
        // Rotating file sink (10MB max, 5 files)
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_file, 1024 * 1024 * 10, 5);
        file_sink->set_level(spdlog::level::trace); // File gets everything
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
        sinks.push_back(file_sink);
        
        // Create logger
        logger_ = std::make_shared<spdlog::logger>("graphdb", sinks.begin(), sinks.end());
        
        // Set log level
        spdlog::level::level_enum level = spdlog::level::info;
        if (log_level == "trace") level = spdlog::level::trace;
        else if (log_level == "debug") level = spdlog::level::debug;
        else if (log_level == "info") level = spdlog::level::info;
        else if (log_level == "warn") level = spdlog::level::warn;
        else if (log_level == "error") level = spdlog::level::err;
        else if (log_level == "critical") level = spdlog::level::critical;
        
        logger_->set_level(level);
        logger_->flush_on(spdlog::level::err);
        
        // Register as default logger
        spdlog::register_logger(logger_);
        spdlog::set_default_logger(logger_);
        
        // Log initialization
        logger_->info("Logger initialized - level: {}, file: {}", log_level, log_file);
        
    } catch (const std::exception& e) {
        std::cerr << "Logger initialization failed: " << e.what() << std::endl;
        
        // Fallback to basic console logger
        logger_ = spdlog::stdout_color_mt("graphdb_fallback");
        logger_->error("Failed to initialize full logger, using console fallback: {}", e.what());
    }
}

std::shared_ptr<spdlog::logger> Logger::get() {
    if (!logger_) {
        init(); // Initialize with defaults if not already done
    }
    return logger_;
}

}  // namespace graphdb::util