#include "piece_pool.hpp"
#include "types.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <vector>

extern const std::map<char, Piece> char_to_piece;
extern const std::map<Piece, char> piece_to_char;

PiecePool::PiecePool() : rng(std::random_device{}()) {}

// Initialize the pool from the FEN string part (e.g., "R2A2...n2b2")
void PiecePool::from_string(std::string_view pool_str) {
    counts.clear();
    if (pool_str.length() % 2 != 0) {
        std::cerr << "Warning: Malformed piece pool string: " << pool_str << std::endl;
        return;
    }

    for (size_t i = 0; i < pool_str.length(); i += 2) {
        char piece_char = pool_str[i];
        char count_char = pool_str[i + 1];

        if (char_to_piece.contains(piece_char) && isdigit(count_char)) {
            counts[char_to_piece.at(piece_char)] = count_char - '0';
        }
        else {
            std::cerr << "Warning: Skipping invalid entry in piece pool string: " << piece_char << count_char << std::endl;
        }
    }
}

// Generate the piece pool string in the new FEN format (mixed red and black pieces)
std::string PiecePool::to_string() const {
    std::string result;
    
    // Define the order for mixed red and black pieces
    std::vector<Piece> order = {
        Piece::RED_ROOK, Piece::BLK_ROOK,
        Piece::RED_ADVISOR, Piece::BLK_ADVISOR,
        Piece::RED_CANNON, Piece::BLK_CANNON,
        Piece::RED_KNIGHT, Piece::BLK_KNIGHT,
        Piece::RED_BISHOP, Piece::BLK_BISHOP,
        Piece::RED_PAWN, Piece::BLK_PAWN
    };
    
    for (const auto& piece : order) {
        auto it = counts.find(piece);
        if (it != counts.end() && it->second > 0) {
            result += piece_to_char.at(piece);
            result += std::to_string(it->second);
        }
    }
    
    return result;
}

// Draws a random piece of a given color from the pool and decrements its count.
std::optional<Piece> PiecePool::draw_random_piece(Color color) {
    std::vector<Piece> available_pieces;
    for (const auto& [piece, count] : counts) {
        if (count > 0 && char_to_piece.contains(piece_to_char.at(piece))) {
            bool is_red = isupper(piece_to_char.at(piece));
            if ((color == Color::RED && is_red) || (color == Color::BLACK && !is_red)) {
                for (int i = 0; i < count; ++i) {
                    available_pieces.push_back(piece);
                }
            }
        }
    }

    if (available_pieces.empty()) {
        return std::nullopt; // No pieces left for this color
    }

    std::uniform_int_distribution<size_t> dist(0, available_pieces.size() - 1);
    Piece drawn_piece = available_pieces[dist(rng)];

    counts[drawn_piece]--;
    return drawn_piece;
}

// For debugging or logging.
void PiecePool::print_pool() const {
    std::cout << "Current Piece Pool:" << std::endl;
    for (const auto& [piece, count] : counts) {
        if (count > 0)
            std::cout << "  " << piece_to_char.at(piece) << ": " << count << std::endl;
    }
} 