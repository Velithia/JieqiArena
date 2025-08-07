#include "move_validator.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>

// Initialize the static initial board layout
const Board MoveValidator::initial_board_layout = {
    {Piece::BLK_ROOK, Piece::BLK_KNIGHT, Piece::BLK_BISHOP, Piece::BLK_ADVISOR, Piece::BLK_KING,
     Piece::BLK_ADVISOR, Piece::BLK_BISHOP, Piece::BLK_KNIGHT, Piece::BLK_ROOK},
    {Piece::EMPTY, Piece::EMPTY, Piece::EMPTY, Piece::EMPTY, Piece::EMPTY, Piece::EMPTY,
     Piece::EMPTY, Piece::EMPTY, Piece::EMPTY},
    {Piece::EMPTY, Piece::BLK_CANNON, Piece::EMPTY, Piece::EMPTY, Piece::EMPTY, Piece::EMPTY,
     Piece::EMPTY, Piece::BLK_CANNON, Piece::EMPTY},
    {Piece::BLK_PAWN, Piece::EMPTY, Piece::BLK_PAWN, Piece::EMPTY, Piece::BLK_PAWN, Piece::EMPTY,
     Piece::BLK_PAWN, Piece::EMPTY, Piece::BLK_PAWN},
    {Piece::EMPTY, Piece::EMPTY, Piece::EMPTY, Piece::EMPTY, Piece::EMPTY, Piece::EMPTY,
     Piece::EMPTY, Piece::EMPTY, Piece::EMPTY},
    {Piece::EMPTY, Piece::EMPTY, Piece::EMPTY, Piece::EMPTY, Piece::EMPTY, Piece::EMPTY,
     Piece::EMPTY, Piece::EMPTY, Piece::EMPTY},
    {Piece::RED_PAWN, Piece::EMPTY, Piece::RED_PAWN, Piece::EMPTY, Piece::RED_PAWN, Piece::EMPTY,
     Piece::RED_PAWN, Piece::EMPTY, Piece::RED_PAWN},
    {Piece::EMPTY, Piece::RED_CANNON, Piece::EMPTY, Piece::EMPTY, Piece::EMPTY, Piece::EMPTY,
     Piece::EMPTY, Piece::RED_CANNON, Piece::EMPTY},
    {Piece::EMPTY, Piece::EMPTY, Piece::EMPTY, Piece::EMPTY, Piece::EMPTY, Piece::EMPTY,
     Piece::EMPTY, Piece::EMPTY, Piece::EMPTY},
    {Piece::RED_ROOK, Piece::RED_KNIGHT, Piece::RED_BISHOP, Piece::RED_ADVISOR, Piece::RED_KING,
     Piece::RED_ADVISOR, Piece::RED_BISHOP, Piece::RED_KNIGHT, Piece::RED_ROOK}};

MoveValidator::MoveValidator() = default;

std::pair<int, int> MoveValidator::coord_to_pos(const std::string &coord) {
    if (coord.length() != 2) return {-1, -1};
    int col = coord[0] - 'a';
    int row = 9 - (coord[1] - '0');
    if (row < 0 || row > 9 || col < 0 || col > 8) return {-1, -1};
    return {row, col};
}

std::optional<Color> MoveValidator::get_piece_color(Piece p) {
    if (p == Piece::EMPTY || p == Piece::HIDDEN) return std::nullopt;
    if (p >= Piece::RED_KING && p <= Piece::RED_PAWN) return Color::RED;
    return Color::BLACK;
}

bool MoveValidator::is_revealed(Piece p) {
    return p != Piece::HIDDEN && p != Piece::EMPTY;
}

Piece MoveValidator::get_base_piece_type(Piece p) {
    if (p >= Piece::BLK_KING && p <= Piece::BLK_PAWN) {
        return static_cast<Piece>(static_cast<int>(p) - 7);
    }
    return p;
}

int MoveValidator::count_pieces_between(int r1, int c1, int r2, int c2, const Board &board) const {
    int count = 0;
    if (r1 == r2) {  // Horizontal
        for (int c = std::min(c1, c2) + 1; c < std::max(c1, c2); ++c) {
            if (board[r1][c] != Piece::EMPTY) count++;
        }
    } else if (c1 == c2) {  // Vertical
        for (int r = std::min(r1, r2) + 1; r < std::max(r1, r2); ++r) {
            if (board[r][c1] != Piece::EMPTY) count++;
        }
    }
    return count;
}

std::optional<std::pair<int, int>> MoveValidator::find_king(Color king_color,
                                                            const Board &board) const {
    Piece king_to_find = (king_color == Color::RED) ? Piece::RED_KING : Piece::BLK_KING;
    for (int r = 0; r < 10; ++r) {
        for (int c = 0; c < 9; ++c) {
            if (board[r][c] == king_to_find) {
                return {{r, c}};
            }
        }
    }
    return std::nullopt;
}

bool MoveValidator::is_move_mechanically_valid(int r1, int c1, int r2, int c2,
                                               const Board &board) const {
    Piece moving_piece = board[r1][c1];
    if (moving_piece == Piece::EMPTY) return false;

    Piece target_piece = board[r2][c2];
    auto moving_color = get_piece_color(moving_piece);
    if (!moving_color && moving_piece != Piece::HIDDEN) return false;  // Should not happen

    // Determine the effective role for hidden pieces
    Piece effective_role_piece = moving_piece;
    if (moving_piece == Piece::HIDDEN) {
        effective_role_piece = initial_board_layout[r1][c1];
        // In Jieqi, the moving color of a hidden piece is determined by its
        // starting position. Red is on rows 0-4, Black on rows 5-9.
        moving_color = (r1 > 4) ? Color::RED : Color::BLACK;
    }

    // A piece cannot capture a piece of the same color
    if (is_revealed(target_piece)) {
        auto target_color = get_piece_color(target_piece);
        if (target_color && target_color == moving_color) {
            return false;
        }
    }

    int dRow = std::abs(r1 - r2);
    int dCol = std::abs(c1 - c2);

    switch (get_base_piece_type(effective_role_piece)) {
        case Piece::RED_KING: {
            int palace_top = (moving_color == Color::RED) ? 7 : 0;
            int palace_bottom = (moving_color == Color::RED) ? 9 : 2;
            return (dRow + dCol == 1) && (c2 >= 3 && c2 <= 5) &&
                   (r2 >= palace_top && r2 <= palace_bottom);
        }
        case Piece::RED_ADVISOR: {
            if (is_revealed(moving_piece)) {  // Revealed advisor can leave palace
                return dRow == 1 && dCol == 1;
            }
            // Hidden advisor is confined to palace
            int palace_top = (moving_color == Color::RED) ? 7 : 0;
            int palace_bottom = (moving_color == Color::RED) ? 9 : 2;
            return dRow == 1 && dCol == 1 && (c2 >= 3 && c2 <= 5) &&
                   (r2 >= palace_top && r2 <= palace_bottom);
        }
        case Piece::RED_BISHOP: {  // Elephant
            if (dRow != 2 || dCol != 2) return false;
            // Check for blocking piece (eye)
            if (board[r1 + (r2 - r1) / 2][c1 + (c2 - c1) / 2] != Piece::EMPTY) return false;
            // Revealed elephants can cross the river. Hidden ones cannot.
            if (!is_revealed(moving_piece)) {
                bool crosses_river = (moving_color == Color::RED && r2 <= 4) ||
                                     (moving_color == Color::BLACK && r2 >= 5);
                if (crosses_river) return false;
            }
            return true;
        }
        case Piece::RED_KNIGHT: {
            if (!((dRow == 2 && dCol == 1) || (dRow == 1 && dCol == 2))) return false;
            // Check for blocking piece (leg)
            int leg_r = r1 + (dRow == 2 ? (r2 - r1) / 2 : 0);
            int leg_c = c1 + (dCol == 2 ? (c2 - c1) / 2 : 0);
            return board[leg_r][leg_c] == Piece::EMPTY;
        }
        case Piece::RED_ROOK: {
            if (dRow > 0 && dCol > 0) return false;
            return count_pieces_between(r1, c1, r2, c2, board) == 0;
        }
        case Piece::RED_CANNON: {
            if (dRow > 0 && dCol > 0) return false;
            int between = count_pieces_between(r1, c1, r2, c2, board);
            if (target_piece != Piece::EMPTY) {  // Capturing
                return between == 1;
            } else {  // Moving
                return between == 0;
            }
        }
        case Piece::RED_PAWN: {
            bool has_crossed_river = (moving_color == Color::RED && r1 <= 4) ||
                                     (moving_color == Color::BLACK && r1 >= 5);
            int forward_dir = (moving_color == Color::RED) ? -1 : 1;

            if (r2 - r1 == forward_dir && dCol == 0) return true;          // Forward move
            if (has_crossed_river && dRow == 0 && dCol == 1) return true;  // Sideways move
            return false;
        }
        default:
            return false;
    }
}

bool MoveValidator::is_in_check(Color king_color, const Board &board) const {
    auto king_pos_opt = find_king(king_color, board);
    if (!king_pos_opt) return true;  // King is captured, which is a game-ending state.
    auto [king_r, king_c] = *king_pos_opt;

    Color opponent_color = (king_color == Color::RED) ? Color::BLACK : Color::RED;

    for (int r = 0; r < 10; ++r) {
        for (int c = 0; c < 9; ++c) {
            Piece p = board[r][c];
            if (!is_revealed(p)) continue;

            auto piece_color = get_piece_color(p);
            if (piece_color == opponent_color) {
                // Special "Flying King (General)" rule
                if (get_base_piece_type(p) == Piece::RED_KING) {
                    if (c == king_c && count_pieces_between(r, c, king_r, king_c, board) == 0) {
                        return true;
                    }
                } else {
                    if (is_move_mechanically_valid(r, c, king_r, king_c, board)) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool MoveValidator::would_be_in_check_after_move(int r1, int c1, int r2, int c2, Color moving_color,
                                                 const Board &board) const {
    Board temp_board = board;
    temp_board[r2][c2] = temp_board[r1][c1];
    temp_board[r1][c1] = Piece::EMPTY;
    return is_in_check(moving_color, temp_board);
}

bool MoveValidator::is_move_legal(const std::string &move_str, Color moving_color,
                                  const Board &board) const {
    if (move_str.length() < 4) return false;
    auto [r1, c1] = coord_to_pos(move_str.substr(0, 2));
    auto [r2, c2] = coord_to_pos(move_str.substr(2, 2));
    if (r1 == -1 || r2 == -1) return false;

    Piece moving_piece = board[r1][c1];
    if (moving_piece == Piece::EMPTY) return false;

    // Check if the piece being moved belongs to the current player
    if (is_revealed(moving_piece)) {
        if (get_piece_color(moving_piece) != moving_color) return false;
    } else {  // Hidden piece
        if (((r1 > 4) ? Color::RED : Color::BLACK) != moving_color) return false;
    }

    if (!is_move_mechanically_valid(r1, c1, r2, c2, board)) {
        return false;
    }

    if (would_be_in_check_after_move(r1, c1, r2, c2, moving_color, board)) {
        return false;
    }

    return true;
}

bool MoveValidator::is_checkmate_or_stalemate(Color player_to_move, const Board &board) const {
    // Iterate over all pieces of the player
    for (int r1 = 0; r1 < 10; ++r1) {
        for (int c1 = 0; c1 < 9; ++c1) {
            Piece p = board[r1][c1];
            if (p == Piece::EMPTY) continue;

            bool is_player_piece = false;
            if (is_revealed(p)) {
                if (get_piece_color(p) == player_to_move) is_player_piece = true;
            } else {  // Hidden piece
                if (((r1 <= 4) ? Color::RED : Color::BLACK) == player_to_move)
                    is_player_piece = true;
            }

            if (is_player_piece) {
                // Check all possible moves for this piece
                for (int r2 = 0; r2 < 10; ++r2) {
                    for (int c2 = 0; c2 < 9; ++c2) {
                        if (is_move_mechanically_valid(r1, c1, r2, c2, board)) {
                            if (!would_be_in_check_after_move(r1, c1, r2, c2, player_to_move,
                                                              board)) {
                                // Found at least one legal move
                                return false;
                            }
                        }
                    }
                }
            }
        }
    }
    // No legal moves found for any piece
    return true;
}