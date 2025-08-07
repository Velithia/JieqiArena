#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "engine_process.hpp"
#include "logger.hpp"

// --- Engine Abstraction ---

class Engine {
   private:
    EngineProcess process;
    std::string name;
    Logger logger;
    int last_eval_cp = 0;          // Last reported evaluation in centipawns
    bool last_eval_has_score = false;  // Whether a score was parsed in the last search

   public:
    Engine(std::string name, int job_id = 0);

    bool start(const std::string &path);
    void stop();
    const std::string &get_name() const;
    void set_position(std::string_view fen, const std::vector<std::string> &moves);

    std::string go(const std::string &go_command, bool is_primary_game);

    // Apply UCI options to the engine process
    void apply_uci_options(const std::string &options_str);

    // Accessors for last evaluation
    int get_last_eval_cp() const;
    bool has_last_eval() const;
};