#pragma once

#include <map>

// --- Core Data Types ---

// Enum for player color, using enum class for type safety.
enum class Color { RED, BLACK, NONE };

// Enum for piece types. 'x' represents a hidden piece.
enum class Piece {
    RED_KING,
    RED_ADVISOR,
    RED_BISHOP,
    RED_KNIGHT,
    RED_ROOK,
    RED_CANNON,
    RED_PAWN,
    BLK_KING,
    BLK_ADVISOR,
    BLK_BISHOP,
    BLK_KNIGHT,
    BLK_ROOK,
    BLK_CANNON,
    BLK_PAWN,
    HIDDEN,
    EMPTY
};

// Maps to convert between char and Piece enum.
extern const std::map<char, Piece> char_to_piece;
extern const std::map<Piece, char> piece_to_char;