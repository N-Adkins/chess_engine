#include "board.hpp"

#include <bit>

constexpr int TEMPO_SCORE = 10;

enum class Piece {
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King,
};

constexpr int VALUES[] = {100, 320, 330, 500, 900, 0};
constexpr int BISHOP_PAIR = 30;

static int addPieceScores(std::uint64_t bitboard, Piece piece) {
    int count = std::popcount(bitboard);
    return count * VALUES[static_cast<int>(piece)];
}

static int evaluateSide(const Board& board, bool white_side) {
    int score = 0;
    std::uint64_t side = white_side ? board.white : board.black;

    score += addPieceScores(board.pawn & side, Piece::Pawn);
    score += addPieceScores(board.knight & side, Piece::Knight);
    score += addPieceScores(board.bishop & side, Piece::Bishop);
    score += addPieceScores(board.rook & side, Piece::Rook);
    score += addPieceScores(board.queen & side, Piece::Queen);
    score += addPieceScores(board.king & side, Piece::King);

    if (std::popcount(board.bishop & side) >= 2) {
        score += BISHOP_PAIR;
    }

    return score;
}

int Board::evaluate() const {
    const int white_score = evaluateSide(*this, true);
    const int black_score = evaluateSide(*this, false);
    const int score = white_score - black_score;
    return score + (white_turn ? TEMPO_SCORE : -TEMPO_SCORE);
}