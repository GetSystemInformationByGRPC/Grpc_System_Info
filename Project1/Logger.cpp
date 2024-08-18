#include "Logger.h"

// Initialize static members
std::shared_ptr<spdlog::logger> Logger::fileLogger_ = nullptr;
std::shared_ptr<spdlog::logger> Logger::consoleLogger_ = nullptr;

void Logger::creator() {
    // Create and configure file logger
    if (fileLogger_ == nullptr) {
        // ایجاد یک rotating_file_sink با حداکثر حجم 5 مگابایت و نگهداری حداکثر 3 فایل
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/logfile.log", 1024 * 1024 * 5, 3);
        fileLogger_ = std::make_shared<spdlog::logger>("file_logger", file_sink);

        fileLogger_->set_level(spdlog::level::debug);
        fileLogger_->flush_on(spdlog::level::debug);
    }

    // Create and configure console logger
    if (consoleLogger_ == nullptr) {
        consoleLogger_ = spdlog::stdout_color_mt("console_logger");
        consoleLogger_->set_level(spdlog::level::debug);
    }
}

void Logger::debug(const std::string& message, bool logToFile, bool logToConsole) {
    if (logToFile && fileLogger_ == nullptr) Logger::creator();
    if (logToFile && fileLogger_) {
        fileLogger_->debug(message);
    }

    if (logToConsole && consoleLogger_ == nullptr) Logger::creator();
    if (logToConsole && consoleLogger_) {
        consoleLogger_->debug(message);
    }
}

void Logger::warning(const std::string& message, bool logToFile, bool logToConsole) {
    if (logToFile && fileLogger_ == nullptr) Logger::creator();
    if (logToFile && fileLogger_) {
        fileLogger_->warn(message);
    }


    if (logToConsole && consoleLogger_ == nullptr) Logger::creator();
    if (logToConsole && consoleLogger_) {
        consoleLogger_->warn(message);
    }
}

void Logger::error(const std::string& message, bool logToFile, bool logToConsole) {

    if (logToFile && fileLogger_ == nullptr) Logger::creator();
    if (logToFile && fileLogger_) {
        fileLogger_->error(message);
    }

    if (logToConsole && consoleLogger_ == nullptr) Logger::creator();
    if (logToConsole && consoleLogger_) {
        consoleLogger_->error(message);
    }
}