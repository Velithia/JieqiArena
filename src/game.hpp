#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "engine.hpp"
#include "move_validator.hpp"
#include "piece_pool.hpp"
#include "time_manager.hpp"
#include "types.hpp"

// --- Game Logic ---

class Game {
   private:
    Engine &red_engine;
    Engine &black_engine;
    std::string initial_fen;
    MoveValidator validator;

    PiecePool piece_pool;
    std::vector<std::vector<Piece>> board;  // 10 rows, 9 columns
    Color current_turn = Color::RED;

    // Three different move histories for different perspectives
    std::vector<std::string> move_history_true;   // God's view - complete information
    std::vector<std::string> move_history_red;    // Red's view - hides Black's hidden captures
    std::vector<std::string> move_history_black;  // Black's view - hides Red's hidden captures

    // Map to store position history for 3-fold repetition check.
    // Key is a FEN string representing the board and side to move.
    std::map<std::string, int> position_history;

    std::optional<TimeManager> time_manager;

   public:
    Game(Engine &r_eng, Engine &b_eng, std::string_view fen,
         std::optional<TimeControl> tc = std::nullopt, int timeout_buffer_ms = 5000);

    // Parses the full FEN string to set up the board and piece pool.
    void parse_fen(std::string_view fen);
    Piece get_piece_at_coord(const std::string &coord);
    void set_piece_at_coord(const std::string &coord, Piece p);

    // Added is_primary_game parameter
    Color run(bool is_primary_game);

    // Generate the complete FEN string in the new format
    std::string generate_fen() const;

   private:
    // Generates the board and turn part of a FEN string for repetition checks.
    std::string generate_fen_board_part() const;
    std::string process_move(const std::string &move_str);

    // Helper functions for managing move histories
    void add_move_to_histories(const std::string &true_move, Color move_color);
    std::vector<std::string> get_moves_for_color(Color color);
};