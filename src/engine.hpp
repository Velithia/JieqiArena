#pragma once

#include "engine_process.hpp"
#include "logger.hpp"
#include <string>
#include <vector>
#include <string_view>

// --- Engine Abstraction ---

class Engine {
private:
    EngineProcess process;
    std::string name;
    Logger logger;

public:
    Engine(std::string name, int job_id = 0);

    bool start(const std::string& path);
    void stop();
    const std::string& get_name() const;
    void set_position(std::string_view fen, const std::vector<std::string>& moves);
    
    std::string go(const std::string& go_command, bool is_primary_game);
    
    // Apply UCI options to the engine process
    void apply_uci_options(const std::string& options_str);
};