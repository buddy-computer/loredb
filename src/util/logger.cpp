#include "logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <vector>

namespace loredb::util {

std::shared_ptr<spdlog::logger> Logger::logger_;

void Logger::init(const std::string& log_file) {
    if (logger_) {
        return; // Already initialized
    }
    
    try {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::warn);

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true);
        file_sink->set_level(spdlog::level::trace);

        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        logger_ = std::make_shared<spdlog::logger>("loredb", sinks.begin(), sinks.end());
        logger_->set_level(spdlog::level::trace);
        
        spdlog::register_logger(logger_);
    } catch (const spdlog::spdlog_ex& ex) {
        // Fallback to basic console logger
        logger_ = spdlog::stdout_color_mt("loredb_fallback");
        logger_->error("Failed to initialize full logger, using console fallback: {}", ex.what());
    }
}

std::shared_ptr<spdlog::logger> Logger::get() {
    if (!logger_) {
        init(); // Initialize with defaults if not already done
    }
    return logger_;
}

} // namespace loredb::util