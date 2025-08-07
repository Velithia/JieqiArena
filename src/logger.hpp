#pragma once

#include <format>
#include <fstream>
#include <string>

// --- Global Configuration ---
// This class provides global control over logging functionality.
// Usage:
//   LoggerConfig::set_enabled(true);   // Enable all logging
//   LoggerConfig::set_enabled(false);  // Disable all logging
//   bool enabled = LoggerConfig::is_enabled(); // Check current state
class LoggerConfig {
   private:
    static bool enabled;

   public:
    // Enable or disable global logging
    // When disabled, no log files will be created and no logging will occur
    static void set_enabled(bool enable);

    // Check if logging is currently enabled
    // Returns true if logging is enabled, false otherwise
    static bool is_enabled();
};

// --- File Logging ---
// Logger class for engine communication debugging.
// Respects the global LoggerConfig setting - if logging is disabled,
// no files will be created and no logging will occur.

class Logger {
   private:
    std::ofstream log_file;
    std::string engine_name;

   public:
    // Create a logger for the specified engine name
    // If global logging is disabled, no log file will be created
    Logger(const std::string &name, int job_id = 0);
    ~Logger();

    // Log a message sent to the engine
    // Only logs if global logging is enabled
    void log_to_engine(const std::string &message);

    // Log a message received from the engine
    // Only logs if global logging is enabled
    void log_from_engine(const std::string &message);
};