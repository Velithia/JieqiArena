#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <deque>
#include <atomic>
#include <format>
#include <sstream>
#include <algorithm>

#include "types.hpp"
#include "time_manager.hpp"
#include "logger.hpp"
#include "game.hpp"
#include "protocol.hpp"

// --- Global State for Tournament Configuration ---
std::string g_engine1_path, g_engine2_path;
std::string g_engine1_options, g_engine2_options;
int g_rounds = 10;
int g_concurrency = 2;
TimeControl g_tc = {1000, 1000, 100, 100}; // Default 1s + 0.1s
int g_timeout_buffer_ms = 5000; // Default 5s

// --- Shared Tournament Resources ---
struct GameTask {
    int game_id;
    std::string red_engine_path;
    std::string black_engine_path;
    std::string red_engine_options;
    std::string black_engine_options;
};

std::deque<GameTask> g_game_queue;
std::mutex g_queue_mutex;
std::atomic<double> g_score_engine1(0.0);
std::atomic<double> g_score_engine2(0.0);
std::atomic<int> g_draws(0);
std::atomic<int> g_wins_engine1(0);
std::atomic<int> g_losses_engine1(0);
std::atomic<int> g_games_completed(0); // To track total games finished across all workers
std::atomic<bool> g_stop_match(false);
std::thread g_tournament_thread;

// Global engine management
std::vector<Engine*> g_active_engines;
std::mutex g_engines_mutex;


// --- Game Logic (remains mostly the same, but uses protocol for output) ---

void stop_all_engines() {
    std::lock_guard<std::mutex> lock(g_engines_mutex);
    for (Engine* engine : g_active_engines) {
        engine->stop();
    }
    g_active_engines.clear();
}

Color play_game(const GameTask& task, bool is_primary) {
    Engine red_engine("Red", task.game_id);
    Engine black_engine("Black", task.game_id);

    // Add engines to global list for management
    {
        std::lock_guard<std::mutex> lock(g_engines_mutex);
        g_active_engines.push_back(&red_engine);
        g_active_engines.push_back(&black_engine);
    }

    if (!red_engine.start(task.red_engine_path)) {
        send_info_string(std::format("[Game {}] Failed to start Red engine ({}). Black wins.", task.game_id, task.red_engine_path));
        return Color::BLACK;
    }
    if (!black_engine.start(task.black_engine_path)) {
        send_info_string(std::format("[Game {}] Failed to start Black engine ({}). Red wins.", task.game_id, task.black_engine_path));
        red_engine.stop();
        return Color::RED;
    }

    red_engine.apply_uci_options(task.red_engine_options);
    black_engine.apply_uci_options(task.black_engine_options);

    Color result = Color::NONE;
    try {
        // Check if tournament should be stopped before starting the game
        if (g_stop_match) {
            return Color::NONE;
        }
        
        const std::string initial_fen = "xxxxkxxxx/9/1x5x1/x1x1x1x1x/9/9/X1X1X1X1X/1X5X1/9/XXXXKXXXX w R2r2N2n2B2b2A2a2C2c2P5p5 0 1";
        // Only the primary worker sends the FEN
        if (is_primary) {
            send_to_gui(std::format("info fen {}", initial_fen));
        }
        Game game(red_engine, black_engine, initial_fen, g_tc, g_timeout_buffer_ms);
        // Pass the primary flag to the game
        result = game.run(is_primary);
    } catch (const std::exception& e) {
        send_info_string(std::format("[Game {}] Crashed with exception: {}. Game is a draw.", task.game_id, e.what()));
        result = Color::NONE;
    }

    red_engine.stop();
    black_engine.stop();
    
    // Remove engines from global list
    {
        std::lock_guard<std::mutex> lock(g_engines_mutex);
        g_active_engines.erase(std::remove(g_active_engines.begin(), g_active_engines.end(), &red_engine), g_active_engines.end());
        g_active_engines.erase(std::remove(g_active_engines.begin(), g_active_engines.end(), &black_engine), g_active_engines.end());
    }
    
    return result;
}

void worker(int worker_id) {
    bool is_primary_worker = (worker_id == 0);

    while (true) {
        if (g_stop_match) {
            // No need for info string here, will be spammy if many workers exist
            return;
        }

        GameTask task;
        int total_games;
        {
            std::lock_guard<std::mutex> lock(g_queue_mutex);
            if (g_game_queue.empty()) {
                return;
            }
            task = g_game_queue.front();
            g_game_queue.pop_front();
            total_games = g_rounds * 2; // Get total games count for reporting
        }
        
        send_info_string(std::format("Starting Game {} on worker {} (Primary: {})", task.game_id, worker_id, is_primary_worker));

        // Extract engine names from paths for info engine command
        if (is_primary_worker) {
            std::string red_engine_name = task.red_engine_path.substr(task.red_engine_path.find_last_of("/\\") + 1);
            std::string black_engine_name = task.black_engine_path.substr(task.black_engine_path.find_last_of("/\\") + 1);
            send_engine_info(red_engine_name, black_engine_name);
        }

        // Pass the primary flag to play_game
        Color result = play_game(task, is_primary_worker);
        
        bool e1_was_red = (task.red_engine_path == g_engine1_path && task.red_engine_options == g_engine1_options);
        
        if (result == Color::RED) {
            if (e1_was_red) {
                g_score_engine1 += 1.0;
                g_wins_engine1++;
            } else {
                g_score_engine2 += 1.0;
                g_losses_engine1++;
            }
        } else if (result == Color::BLACK) {
            if (e1_was_red) {
                g_score_engine2 += 1.0;
                g_losses_engine1++;
            } else {
                g_score_engine1 += 1.0;
                g_wins_engine1++;
            }
        } else {
            // Includes Color::NONE for aborted games
            g_score_engine1 += 0.5;
            g_score_engine2 += 0.5;
            g_draws++;
        }
        
        // Increment total games completed and send universal updates
        int completed_count = ++g_games_completed;
        
        send_info_string(std::format("Game {} Finished. Score: E1 {:.1f} - E2 {:.1f} (Draws: {})",
            task.game_id, g_score_engine1.load(), g_score_engine2.load(), g_draws.load()));
        
        // These are global stats, so any worker can send them. The GUI will just update.
        send_to_gui(std::format("info game {}/{}", completed_count, total_games));
        send_to_gui(std::format("info wld {}-{}-{}", g_wins_engine1.load(), g_losses_engine1.load(), g_draws.load()));
    }
}

void run_tournament() {
    g_stop_match = false;
    g_score_engine1 = 0.0;
    g_score_engine2 = 0.0;
    g_draws = 0;
    g_wins_engine1 = 0;
    g_losses_engine1 = 0;
    g_games_completed = 0; // Reset counter

    int total_games = g_rounds * 2;
    send_info_string("Populating game queue...");
    {
        std::lock_guard<std::mutex> lock(g_queue_mutex);
        g_game_queue.clear();
        for (int i = 0; i < g_rounds; ++i) {
            // Game where Engine 1 is Red, Engine 2 is Black
            g_game_queue.push_back({
                i * 2 + 1,
                g_engine1_path, g_engine2_path,
                g_engine1_options, g_engine2_options
            });
            // Game where Engine 2 is Red, Engine 1 is Black
            g_game_queue.push_back({
                i * 2 + 2,
                g_engine2_path, g_engine1_path,
                g_engine2_options, g_engine1_options
            });
        }
    }
    
    send_to_gui(std::format("info game 0/{}", total_games));
    send_to_gui("info wld 0-0-0");
    send_info_string(std::format("Match started with {} worker(s).", g_concurrency));

    std::vector<std::thread> workers;
    for (int i = 0; i < g_concurrency; ++i) {
        // Pass worker_id to the thread constructor
        workers.emplace_back(worker, i);
    }

    for (auto& w : workers) {
        if(w.joinable()) w.join();
    }
    
    if (g_stop_match) {
        send_info_string("Tournament stopped prematurely.");
    } else {
        send_info_string("Tournament finished!");
    }
    // Send final WLD
    send_to_gui(std::format("info wld {}-{}-{}", g_wins_engine1.load(), g_losses_engine1.load(), g_draws.load()));
}


// --- JAI Command Handling ---
void handle_jai() {
    send_to_gui("id name JieqiArena Match Engine");
    send_to_gui("id author Velithia");
    
    send_to_gui("option name Engine1Path type string");
    send_to_gui("option name Engine1Options type string");
    send_to_gui("option name Engine2Path type string");
    send_to_gui("option name Engine2Options type string");
    send_to_gui("option name TotalRounds type spin default 10 min 1 max 1000");
    send_to_gui("option name Concurrency type spin default 2 min 1 max 128");
    send_to_gui("option name MainTimeMs type spin default 1000 min 0 max 3600000");
    send_to_gui("option name IncTimeMs type spin default 0 min 0 max 60000");
    send_to_gui("option name TimeoutBufferMs type spin default 5000 min 0 max 60000");
    send_to_gui("option name Logging type check default false");
    
    send_to_gui("jaiok");
}

void handle_setoption(const std::string& line) {
    std::stringstream ss(line);
    std::string token, name_token, option_name, value_token, option_value;
    ss >> token >> name_token >> option_name >> value_token;
    std::getline(ss, option_value);
    // Trim leading space from value
    if (!option_value.empty() && option_value.front() == ' ') {
        option_value.erase(0, 1);
    }

    if (name_token != "name" || value_token != "value") return;

    if (option_name == "Engine1Path") g_engine1_path = option_value;
    else if (option_name == "Engine2Path") g_engine2_path = option_value;
    else if (option_name == "Engine1Options") g_engine1_options = option_value;
    else if (option_name == "Engine2Options") g_engine2_options = option_value;
    else if (option_name == "TotalRounds") g_rounds = std::stoi(option_value);
    else if (option_name == "Concurrency") g_concurrency = std::stoi(option_value);
    else if (option_name == "MainTimeMs") g_tc.wtime_ms = g_tc.btime_ms = std::stoi(option_value);
    else if (option_name == "IncTimeMs") g_tc.winc_ms = g_tc.binc_ms = std::stoi(option_value);
    else if (option_name == "TimeoutBufferMs") g_timeout_buffer_ms = std::stoi(option_value);
    else if (option_name == "Logging") LoggerConfig::set_enabled(option_value == "true");
}

int main(int argc, char* argv[]) {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::stringstream ss(line);
        std::string command;
        ss >> command;

        if (line.empty()) continue;

        if (command == "jai") {
            handle_jai();
        } else if (command == "setoption") {
            handle_setoption(line);
        } else if (command == "isready") {
            if (g_engine1_path.empty() || g_engine2_path.empty()) {
                send_info_string("Error: Engine paths are not set.");
            } else {
                send_to_gui("readyok");
            }
        } else if (command == "startmatch") {
            if (g_tournament_thread.joinable()) {
                g_tournament_thread.join();
            }
            g_tournament_thread = std::thread(run_tournament);
        } else if (command == "stop") {
            g_stop_match = true;
            // Stop all active engines immediately
            stop_all_engines();
            // Clear the game queue to stop all pending games immediately
            {
                std::lock_guard<std::mutex> lock(g_queue_mutex);
                g_game_queue.clear();
            }
            if (g_tournament_thread.joinable()) {
                g_tournament_thread.join();
            }
        } else if (command == "quit") {
            g_stop_match = true;
            if (g_tournament_thread.joinable()) {
                g_tournament_thread.join();
            }
            break;
        }
    }
    return 0;
}