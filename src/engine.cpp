#include "engine.hpp"
#include "protocol.hpp"
#include <iostream>
#include <sstream>
#include <format>
#include <thread>
#include <chrono>

Engine::Engine(std::string name, int job_id) : name(std::move(name)), logger(name, job_id) {}

bool Engine::start(const std::string& path) {
    // GUI will get this info from JAI Engine, not the child process directly.
    // std::cout << std::format("Starting engine '{}' with command: {}\n", name, path);
    return process.start(path);
}

void Engine::stop() {
    process.write_line("quit");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    process.stop();
}

const std::string& Engine::get_name() const { return name; }

void Engine::set_position(std::string_view fen, const std::vector<std::string>& moves) {
    std::string cmd = std::format("position fen {}", fen);
    if (!moves.empty()) {
        cmd += " moves";
        for (const auto& move : moves) {
            cmd += " " + move;
        }
    }
    logger.log_to_engine(cmd);
    process.write_line(cmd);
}

// Added is_primary_game parameter
std::string Engine::go(const std::string& go_command, bool is_primary_game) {
    logger.log_to_engine(go_command);
    process.write_line(go_command);
    while (true) {
        std::string line = process.read_line();
        logger.log_from_engine(line);

        if (line.empty()) { // Check for empty line / process crash first
            if (!process.is_running()) {
                send_info_string(std::format("Error: Engine {} has stopped responding.", name));
                return "resign";
            }
            continue; // Could be an empty line for other reasons, just wait for more output
        }

        // Forward UCI info lines to the GUI
        if (line.rfind("info", 0) == 0) {
            // Don't forward info string lines (engine's own messages)
            if (line.rfind("info string", 0) != 0) {
                // Conditional send for engine analysis
                if (is_primary_game) {
                    send_to_gui(line); // Pass-through the info line
                }
            }
            continue; // Continue listening
        }

        if (line.rfind("bestmove", 0) == 0) {
            std::stringstream ss(line);
            std::string token, best_move;
            ss >> token >> best_move;
            return best_move;
        }
    }
}