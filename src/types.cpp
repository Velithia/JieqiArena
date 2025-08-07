#include "types.hpp"

// Maps to convert between char and Piece enum.
const std::map<char, Piece> char_to_piece = {
    {'K', Piece::RED_KING},   {'A', Piece::RED_ADVISOR}, {'B', Piece::RED_BISHOP},
    {'N', Piece::RED_KNIGHT}, {'R', Piece::RED_ROOK},    {'C', Piece::RED_CANNON},
    {'P', Piece::RED_PAWN},   {'k', Piece::BLK_KING},    {'a', Piece::BLK_ADVISOR},
    {'b', Piece::BLK_BISHOP}, {'n', Piece::BLK_KNIGHT},  {'r', Piece::BLK_ROOK},
    {'c', Piece::BLK_CANNON}, {'p', Piece::BLK_PAWN},    {'x', Piece::HIDDEN}};

const std::map<Piece, char> piece_to_char = {
    {Piece::RED_KING, 'K'},   {Piece::RED_ADVISOR, 'A'}, {Piece::RED_BISHOP, 'B'},
    {Piece::RED_KNIGHT, 'N'}, {Piece::RED_ROOK, 'R'},    {Piece::RED_CANNON, 'C'},
    {Piece::RED_PAWN, 'P'},   {Piece::BLK_KING, 'k'},    {Piece::BLK_ADVISOR, 'a'},
    {Piece::BLK_BISHOP, 'b'}, {Piece::BLK_KNIGHT, 'n'},  {Piece::BLK_ROOK, 'r'},
    {Piece::BLK_CANNON, 'c'}, {Piece::BLK_PAWN, 'p'},    {Piece::HIDDEN, 'x'}};