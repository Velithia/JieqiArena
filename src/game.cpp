#include "game.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <format>
#include <iostream>
#include <ranges>
#include <sstream>

#include "protocol.hpp"
#include "types.hpp"

extern const std::map<char, Piece> char_to_piece;
extern const std::map<Piece, char> piece_to_char;
extern std::atomic<bool> g_stop_match;

Game::Game(Engine &r_eng, Engine &b_eng, std::string_view fen, std::optional<TimeControl> tc,
           int timeout_buffer_ms)
    : red_engine(r_eng), black_engine(b_eng), initial_fen(fen) {
    if (tc) {
        time_manager.emplace(*tc, timeout_buffer_ms);
    }
    parse_fen(fen);
}

// ... (parse_fen, get_piece_at_coord, set_piece_at_coord remain the same) ...
void Game::parse_fen(std::string_view fen) {
    board.assign(10, std::vector<Piece>(9, Piece::EMPTY));

    auto parts = fen | std::views::split(' ') | std::ranges::to<std::vector<std::string>>();
    if (parts.size() < 3) {
        throw std::runtime_error("Invalid FEN string: not enough parts.");
    }

    // Part 1: Board position
    std::string_view board_part = parts[0];
    int row = 0, col = 0;
    for (char c : board_part) {
        if (c == '/') {
            row++;
            col = 0;
        } else if (isdigit(c)) {
            col += c - '0';
        } else {
            if (col < 9 && row < 10) {
                board[row][col] = (c == 'x' || c == 'X') ? Piece::HIDDEN : char_to_piece.at(c);
                col++;
            }
        }
    }

    // Part 2: Side to move
    std::string_view turn_part = parts[1];
    current_turn = (turn_part == "w") ? Color::RED : Color::BLACK;

    // Part 3: Piece Pool
    std::string_view pool_part = parts[2];
    piece_pool.from_string(pool_part);

    // Record the initial position for repetition check
    std::string fen_key =
        generate_fen_board_part() + " " + (current_turn == Color::RED ? 'w' : 'b');
    position_history[fen_key]++;
}

Piece Game::get_piece_at_coord(const std::string &coord) {
    if (coord.length() != 2) return Piece::EMPTY;
    int col = coord[0] - 'a';
    int row = 9 - (coord[1] - '0');
    if (row < 0 || row > 9 || col < 0 || col > 8) return Piece::EMPTY;
    return board[row][col];
}

void Game::set_piece_at_coord(const std::string &coord, Piece p) {
    if (coord.length() != 2) return;
    int col = coord[0] - 'a';
    int row = 9 - (coord[1] - '0');
    if (row < 0 || row > 9 || col < 0 || col > 8) return;
    board[row][col] = p;
}

Color Game::run(bool is_primary_game) {
    for (int move_count = 1;; ++move_count) {
        if (move_count > 300) {
            send_info_string("Game ends in a draw (move limit reached).");
            if (is_primary_game) send_to_gui("info result 1/2-1/2");
            return Color::NONE;
        }

        if (g_stop_match) {
            return Color::NONE;
        }

        Engine &current_engine = (current_turn == Color::RED) ? red_engine : black_engine;
        Engine &opponent_engine = (current_turn == Color::RED) ? black_engine : red_engine;

        auto moves_for_current_player = get_moves_for_color(current_turn);
        current_engine.set_position(initial_fen, moves_for_current_player);

        std::string go_command = time_manager ? time_manager->get_go_command() : "go movetime 2000";

        auto start_time = std::chrono::steady_clock::now();
        std::string best_move_str = current_engine.go(go_command, is_primary_game);
        auto end_time = std::chrono::steady_clock::now();

        long long elapsed_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

        // --- RESIGNATION / CRASH CHECK ---
        if (best_move_str == "resign" || best_move_str.empty() || best_move_str == "(none)") {
            std::string reason = "resigns or crashed";
            if (best_move_str == "(none)") reason = "returned no move";

            send_info_string(std::format("{} {}. {} wins.", current_engine.get_name(), reason,
                                         opponent_engine.get_name()));
            if (is_primary_game)
                send_to_gui(
                    std::format("info result {}", (current_turn == Color::RED ? "0-1" : "1-0")));
            return (current_turn == Color::RED) ? Color::BLACK : Color::RED;
        }

        // --- ILLEGAL MOVE VALIDATION ---
        if (!validator.is_move_legal(best_move_str, current_turn, board)) {
            validator.is_move_legal(best_move_str, current_turn, board);
            send_info_string(std::format("{} made an illegal move ({}). {} wins.",
                                         current_engine.get_name(), best_move_str,
                                         opponent_engine.get_name()));
            if (is_primary_game)
                send_to_gui(
                    std::format("info result {}", (current_turn == Color::RED ? "0-1" : "1-0")));
            return (current_turn == Color::RED) ? Color::BLACK : Color::RED;
        }

        // --- TIME CHECK ---
        if (time_manager) {
            time_manager->update(current_turn, elapsed_ms);
            if (time_manager->is_out_of_time(current_turn)) {
                send_info_string(std::format("{} loses on time. {} wins.",
                                             current_engine.get_name(),
                                             opponent_engine.get_name()));
                if (is_primary_game)
                    send_to_gui(std::format("info result {}",
                                            (current_turn == Color::RED ? "0-1" : "1-0")));
                return (current_turn == Color::RED) ? Color::BLACK : Color::RED;
            }
        }

        // --- PROCESS VALID MOVE ---
        std::string augmented_move = process_move(best_move_str);

        if (is_primary_game) {
            send_to_gui(std::format("info move {} time {}", augmented_move, elapsed_ms));
        }

        add_move_to_histories(augmented_move, current_turn);

        // --- SWITCH TURN & CHECK FOR ENDGAME ---
        current_turn = (current_turn == Color::RED) ? Color::BLACK : Color::RED;

        // --- CHECK FOR CHECKMATE/STALEMATE ---
        if (validator.is_checkmate_or_stalemate(current_turn, board)) {
            if (validator.is_in_check(current_turn, board)) {
                // Checkmate
                send_info_string(std::format("{} is in checkmate. {} wins.",
                                             (current_turn == Color::RED ? "Red" : "Black"),
                                             opponent_engine.get_name()));
                if (is_primary_game)
                    send_to_gui(std::format("info result {}",
                                            (current_turn == Color::RED ? "0-1" : "1-0")));
                return (current_turn == Color::RED) ? Color::BLACK : Color::RED;
            } else {
                // Stalemate
                send_info_string(std::format("{} is stalemated. Game is a draw.",
                                             (current_turn == Color::RED ? "Red" : "Black")));
                if (is_primary_game) send_to_gui("info result 1/2-1/2");
                return Color::NONE;
            }
        }

        // --- REPETITION CHECK ---
        std::string fen_key =
            generate_fen_board_part() + " " + (current_turn == Color::RED ? 'w' : 'b');
        position_history[fen_key]++;
        if (position_history[fen_key] >= 3) {
            send_info_string("Game ends in a draw by 3-fold repetition.");
            if (is_primary_game) send_to_gui("info result 1/2-1/2");
            return Color::NONE;
        }
    }
}

// ... (generate_fen_board_part, generate_fen, process_move,
// add_move_to_histories, get_moves_for_color remain the same) ...
std::string Game::generate_fen_board_part() const {
    std::stringstream ss;
    for (int r = 0; r < 10; ++r) {
        int empty_count = 0;
        for (int c = 0; c < 9; ++c) {
            Piece p = board[r][c];
            if (p == Piece::EMPTY) {
                empty_count++;
            } else {
                if (empty_count > 0) {
                    ss << empty_count;
                    empty_count = 0;
                }
                ss << piece_to_char.at(p);
            }
        }
        if (empty_count > 0) {
            ss << empty_count;
        }
        if (r < 9) {
            ss << '/';
        }
    }
    return ss.str();
}

// Generate the complete FEN string in the new format
std::string Game::generate_fen() const {
    std::string fen = generate_fen_board_part();
    fen += " ";
    fen += (current_turn == Color::RED ? "w" : "b");
    fen += " ";
    fen += piece_pool.to_string();
    fen += " 0 1";  // Move number and half-move clock
    return fen;
}

std::string Game::process_move(const std::string &move_str) {
    if (move_str.length() != 4) return move_str;

    std::string from_coord = move_str.substr(0, 2);
    std::string to_coord = move_str.substr(2, 4);

    Piece moving_piece_type = get_piece_at_coord(from_coord);
    Piece target_square_piece_type = get_piece_at_coord(to_coord);

    std::string augmented_move = move_str;
    std::optional<Piece> flipped_piece = std::nullopt;

    // A. Handle flip (moving a hidden piece)
    if (moving_piece_type == Piece::HIDDEN) {
        flipped_piece = piece_pool.draw_random_piece(current_turn);
        if (flipped_piece) {
            augmented_move += piece_to_char.at(*flipped_piece);
        } else {
            send_info_string(std::format("CRITICAL: Piece pool is empty for {}. Cannot flip.",
                                         (current_turn == Color::RED ? "Red" : "Black")));
            // As a fallback, maybe make it a pawn? This state should ideally not be
            // reached.
            flipped_piece = (current_turn == Color::RED) ? Piece::RED_PAWN : Piece::BLK_PAWN;
            augmented_move += piece_to_char.at(*flipped_piece);
        }
    }

    // B. Handle capture of a hidden piece
    if (target_square_piece_type == Piece::HIDDEN) {
        Color opponent_color = (current_turn == Color::RED) ? Color::BLACK : Color::RED;
        auto captured_hidden_piece = piece_pool.draw_random_piece(opponent_color);
        if (captured_hidden_piece) {
            augmented_move += piece_to_char.at(*captured_hidden_piece);
        } else {
            send_info_string("Warning: Opponent piece pool is empty for capture simulation.");
        }
    }

    // C. Update the internal board state with ground truth
    Piece final_moving_piece = flipped_piece.value_or(moving_piece_type);
    set_piece_at_coord(to_coord, final_moving_piece);
    set_piece_at_coord(from_coord, Piece::EMPTY);

    return augmented_move;
}

// Helper function to add a move to all histories with proper information hiding
void Game::add_move_to_histories(const std::string &true_move, Color move_color) {
    move_history_true.push_back(true_move);

    std::string red_move = true_move;
    std::string black_move = true_move;

    if (true_move.length() > 4) {
        char last_char = true_move.back();
        if (move_color == Color::BLACK && isupper(last_char)) {
            red_move.pop_back();
        } else if (move_color == Color::RED && islower(last_char)) {
            black_move.pop_back();
        }
    }

    move_history_red.push_back(red_move);
    move_history_black.push_back(black_move);
}

std::vector<std::string> Game::get_moves_for_color(Color color) {
    return (color == Color::RED) ? move_history_red : move_history_black;
}