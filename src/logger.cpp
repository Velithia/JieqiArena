#include "logger.hpp"

// Initialize static member
bool LoggerConfig::enabled = true;

void LoggerConfig::set_enabled(bool enable) {
    enabled = enable;
}

bool LoggerConfig::is_enabled() {
    return enabled;
}

Logger::Logger(const std::string &name, int job_id) : engine_name(name) {
    // Only create log file if logging is enabled
    if (!LoggerConfig::is_enabled()) {
        return;
    }

    std::string filename = std::format("engine_debug_{}_job{}.log", name, job_id);
    log_file.open(filename, std::ios::app);
    if (log_file.is_open()) {
        log_file << std::format("[{}] Engine debug log started\n", engine_name) << std::flush;
    }
}

Logger::~Logger() {
    if (log_file.is_open()) {
        log_file.close();
    }
}

void Logger::log_to_engine(const std::string &message) {
    // Only log if logging is enabled and file is open
    if (LoggerConfig::is_enabled() && log_file.is_open()) {
        log_file << std::format("[TO {}]: {}\n", engine_name, message) << std::flush;
    }
}

void Logger::log_from_engine(const std::string &message) {
    // Only log if logging is enabled and file is open
    if (LoggerConfig::is_enabled() && log_file.is_open()) {
        log_file << std::format("[FROM {}]: {}\n", engine_name, message) << std::flush;
    }
}