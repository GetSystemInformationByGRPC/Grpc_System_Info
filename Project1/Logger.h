#ifndef LOGGER_H
#define LOGGER_H

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>

class Logger {
public:
    // Static functions for logging
    static void debug(const std::string& message, bool logToFile=true, bool logToConsole=true);
    static void warning(const std::string& message, bool logToFile=true, bool logToConsole=true);
    static void error(const std::string& message, bool logToFile=true, bool logToConsole=true);

    static void creator();

private:
    // Logger instances for file and console logging
    static std::shared_ptr<spdlog::logger> fileLogger_;
    static std::shared_ptr<spdlog::logger> consoleLogger_;
};

#endif // LOGGER_H
