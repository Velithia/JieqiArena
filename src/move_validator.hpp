#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "types.hpp"

using Board = std::vector<std::vector<Piece>>;

// --- Move Validation Logic ---
// This class encapsulates all the rules for Jieqi move validation.
class MoveValidator {
   public:
    MoveValidator();

    // The main public function to check if a move is fully legal.
    // A move is legal if:
    // 1. It is mechanically valid (follows the piece's movement rules).
    // 2. It does not result in the player's own king being in check.
    bool is_move_legal(const std::string &move_str, Color moving_color, const Board &board) const;

    // Checks if the specified player is currently in check.
    bool is_in_check(Color king_color, const Board &board) const;

    // Checks if the specified player is in checkmate or stalemate.
    // Returns true if the player has no legal moves.
    bool is_checkmate_or_stalemate(Color player_to_move, const Board &board) const;

   private:
    // Helper to convert "a0" style coordinates to (row, col) pair.
    static std::pair<int, int> coord_to_pos(const std::string &coord);
    // Helper to get piece color.
    static std::optional<Color> get_piece_color(Piece p);
    // Helper to check if a piece is revealed.
    static bool is_revealed(Piece p);
    // Helper to get the fundamental type of a piece (e.g., RED_ROOK -> ROOK).
    static Piece get_base_piece_type(Piece p);

    // Checks if a move is mechanically valid, without considering check status.
    bool is_move_mechanically_valid(int r1, int c1, int r2, int c2, const Board &board) const;

    // Simulates a move and checks if it would leave the king in check.
    bool would_be_in_check_after_move(int r1, int c1, int r2, int c2, Color moving_color,
                                      const Board &board) const;

    // Counts pieces between two points on a straight line.
    int count_pieces_between(int r1, int c1, int r2, int c2, const Board &board) const;

    // Finds the position of the specified king.
    std::optional<std::pair<int, int>> find_king(Color king_color, const Board &board) const;

    // A static representation of the initial board layout to determine hidden
    // piece move rules.
    static const Board initial_board_layout;
};