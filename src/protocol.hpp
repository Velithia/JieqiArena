#pragma once

#include <format>
#include <iostream>
#include <mutex>
#include <string>

// Global mutex for thread-safe writing to stdout
extern std::mutex g_gui_mutex;

// Sends a message to the GUI in a thread-safe manner.
// It automatically adds a newline and flushes the stream.
inline void send_to_gui(const std::string &message) {
    std::lock_guard<std::mutex> lock(g_gui_mutex);
    std::cout << message << std::endl;
}

// A helper to send formatted info strings
inline void send_info_string(const std::string &message) {
    send_to_gui(std::format("info string {}", message));
}

// A helper to send engine information
inline void send_engine_info(const std::string &red_engine, const std::string &black_engine) {
    send_to_gui(std::format("info engine {} {}", red_engine, black_engine));
}