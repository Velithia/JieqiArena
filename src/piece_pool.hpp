#pragma once

#include <map>
#include <optional>
#include <random>
#include <string_view>

#include "types.hpp"

// --- Piece Pool ---

// Manages the count of unrevealed pieces for both sides.
class PiecePool {
   private:
    std::map<Piece, int> counts;
    std::mt19937 rng;  // Mersenne Twister random number generator

   public:
    PiecePool();

    // Initialize the pool from the FEN string part (e.g., "R2A2...n2b2")
    void from_string(std::string_view pool_str);

    // Generate the piece pool string in the new FEN format (mixed red and black
    // pieces)
    std::string to_string() const;

    // Draws a random piece of a given color from the pool and decrements its
    // count.
    std::optional<Piece> draw_random_piece(Color color);

    // For debugging or logging.
    void print_pool() const;
};