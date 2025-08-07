#include "engine.hpp"

#include <algorithm>  // for std::find_if
#include <chrono>
#include <format>
#include <iostream>
#include <sstream>
#include <thread>

#include "protocol.hpp"

Engine::Engine(std::string name, int job_id) : name(std::move(name)), logger(name, job_id) {}

bool Engine::start(const std::string &path) {
    // GUI will get this info from JAI Engine, not the child process directly.
    // std::cout << std::format("Starting engine '{}' with command: {}\n", name,
    // path);
    return process.start(path);
}

void Engine::stop() {
    process.write_line("quit");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    process.stop();
}

void Engine::apply_uci_options(const std::string &options_str) {
    if (options_str.empty()) {
        return;
    }

    // Helper lambda to trim whitespace from both ends of a string
    auto trim = [](std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                        [](unsigned char ch) { return !std::isspace(ch); }));
        s.erase(
            std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
            s.end());
    };

    std::string temp_options = options_str;
    size_t current_pos = 0;
    const std::string name_keyword = "name ";

    // The logic is to find "name " to identify the start of each option,
    // then find the next "name " to identify the end of the current option.
    while ((current_pos = temp_options.find(name_keyword, current_pos)) != std::string::npos) {
        // Find the start of the next option to delimit the end of the current one.
        size_t next_name_pos = temp_options.find(name_keyword, current_pos + name_keyword.length());

        // Extract the full block for one option
        std::string block = temp_options.substr(current_pos, next_name_pos - current_pos);

        // Move current_pos for the next iteration
        current_pos = next_name_pos;

        // Now parse the single block
        const std::string value_keyword = " value ";
        size_t value_pos = block.find(value_keyword);

        if (value_pos != std::string::npos) {
            // Extract the name part (between "name " and " value ")
            std::string opt_name =
                block.substr(name_keyword.length(), value_pos - name_keyword.length());

            // Extract the value part (everything after " value ")
            std::string opt_value = block.substr(value_pos + value_keyword.length());

            // Clean up any leading/trailing whitespace
            trim(opt_name);
            trim(opt_value);

            if (!opt_name.empty()) {
                std::string cmd = std::format("setoption name {} value {}", opt_name, opt_value);
                logger.log_to_engine(cmd);
                process.write_line(cmd);
            }
        }

        // If we are at the end, break the loop
        if (next_name_pos == std::string::npos) {
            break;
        }
    }
}

const std::string &Engine::get_name() const {
    return name;
}

void Engine::set_position(std::string_view fen, const std::vector<std::string> &moves) {
    std::string cmd = std::format("position fen {}", fen);
    if (!moves.empty()) {
        cmd += " moves";
        for (const auto &move : moves) {
            cmd += " " + move;
        }
    }
    logger.log_to_engine(cmd);
    process.write_line(cmd);
}

std::string Engine::go(const std::string &go_command, bool is_primary_game) {
    // Reset last eval state for this search
    last_eval_has_score = false;
    last_eval_cp = 0;

    logger.log_to_engine(go_command);
    process.write_line(go_command);
    while (true) {
        std::string line = process.read_line();
        logger.log_from_engine(line);

        if (line.empty()) {  // Check for empty line / process crash first
            if (!process.is_running()) {
                send_info_string(std::format("Error: Engine {} has stopped responding.", name));
                return "resign";
            }
            continue;  // Could be an empty line for other reasons, just wait for more
                       // output
        }

        // Forward UCI info lines to the GUI and parse eval
        if (line.rfind("info", 0) == 0) {
            // Don't forward info string lines (engine's own messages)
            if (line.rfind("info string", 0) != 0) {
                // Parse score: "info ... score cp N" or "info ... score mate M"
                std::stringstream iss(line);
                std::string tok;
                while (iss >> tok) {
                    if (tok == "score") {
                        std::string type; iss >> type; // cp or mate
                        if (type == "cp") {
                            int cp; if (iss >> cp) { last_eval_cp = cp; last_eval_has_score = true; }
                        } else if (type == "mate") {
                            int mate_in; if (iss >> mate_in) {
                                last_eval_cp = (mate_in >= 0) ? 10000 : -10000;
                                last_eval_has_score = true;
                            }
                        }
                    }
                }
                // Conditional send for engine analysis
                if (is_primary_game) {
                    send_to_gui(line);  // Pass-through the info line
                }
            }
            continue;  // Continue listening
        }

        if (line.rfind("bestmove", 0) == 0) {
            std::stringstream ss(line);
            std::string token, best_move;
            ss >> token >> best_move;
            return best_move;
        }
    }
}

int Engine::get_last_eval_cp() const {
    return last_eval_cp;
}

bool Engine::has_last_eval() const {
    return last_eval_has_score;
}