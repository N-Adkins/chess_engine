#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

constexpr std::string_view STARTING_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

enum class MoveFlag : uint8_t {
    Quiet = 0,
    Capture = 1 << 0,
    DoublePush = 1 << 1,
    EnPassant = 1 << 2,
    Castle = 1 << 3,
    Promotion = 1 << 4,
};

struct Move {
    int from, to;
    uint8_t flags;
};

// Magic bitboard data
struct Magic {
    std::uint64_t mask;
    std::uint64_t magic;
    int shift;
    std::vector<std::uint64_t> attacks;
};

struct Board {
    std::uint64_t white = 0, black = 0, pawn = 0, bishop = 0,
            knight = 0, rook = 0, king = 0, queen = 0;

    int half_moves = 0;
    bool white_turn = true;

    static Board initFEN(std::string_view fen);
    std::string toString() const;

    std::uint64_t currentOccupied() const; // Returns bitboard for color of current turn
    std::uint64_t nextOccupied() const; // Returns bitboard for color of next turn

    void pawnMoves(std::vector<Move>& out);
    void knightMoves(std::vector<Move>& out);
    void bishopMoves(std::vector<Move>& out);
    void rookMoves(std::vector<Move>& out);
    void queenMoves(std::vector<Move>& out);
    void kingMoves(std::vector<Move>& out);
    void generatePseudoMoves(std::vector<Move>& out);

private:
    Board() = default;
};

constexpr std::uint64_t FILE_A = 0x0101010101010101ULL;
constexpr std::uint64_t FILE_H = 0x8080808080808080ULL;
constexpr std::uint64_t FILE_B = FILE_A << 1;
constexpr std::uint64_t FILE_G = FILE_H >> 1;
constexpr std::uint64_t RANK_1 = 0xFF;
constexpr std::uint64_t RANK_2 = RANK_1 << 8;
constexpr std::uint64_t RANK_7 = RANK_1 << 48;
constexpr std::uint64_t RANK_8 = 0xFF00000000000000ULL;

consteval std::array<std::uint64_t, 64> makeWhitePawnAttacks() {
    std::array<std::uint64_t, 64> table{};
    for (std::size_t square = 0; square < 64; square++) {
        const auto bit = 1ULL << square;
        const auto northwest = (bit & ~FILE_A) << 7;
        const auto northeast = (bit & ~FILE_H) << 9;
        table[square] = northwest | northeast;
    }
    return table;
}

consteval std::array<std::uint64_t, 64> makeBlackPawnAttacks() {
    std::array<std::uint64_t, 64> table{};
    for (std::size_t square = 0; square < 64; square++) {
        const auto bit = 1ULL << square;
        const auto southwest = (bit & ~FILE_A) >> 9;
        const auto southeast = (bit & ~FILE_H) >> 7;
        table[square] = southwest | southeast;
    }
    return table;
}

consteval std::array<std::uint64_t, 64> makeKnightAttacks() {
    std::array<std::uint64_t, 64> table{};
    for (std::size_t square = 0; square < 64; square++) {
        const auto bit = 1ULL << square;
        std::uint64_t attacks = 0;

        // Up moves
        if ((bit & (FILE_A | FILE_B)) == 0 && (bit & RANK_8) == 0) attacks |= bit << 6;   // two left, one up
        if ((bit & (FILE_G | FILE_H)) == 0 && (bit & RANK_8) == 0) attacks |= bit << 10;  // two right, one up
        if ((bit & FILE_A) == 0 && (bit & (RANK_7 | RANK_8)) == 0) attacks |= bit << 15;  // one left, two up
        if ((bit & FILE_H) == 0 && (bit & (RANK_7 | RANK_8)) == 0) attacks |= bit << 17;  // one right, two up

        // Down moves
        if ((bit & (FILE_G | FILE_H)) == 0 && (bit & RANK_1) == 0) attacks |= bit >> 6;   // two right, one down
        if ((bit & (FILE_A | FILE_B)) == 0 && (bit & RANK_1) == 0) attacks |= bit >> 10;  // two left, one down
        if ((bit & FILE_H) == 0 && (bit & (RANK_1 | RANK_2)) == 0) attacks |= bit >> 15;  // one right, two down
        if ((bit & FILE_A) == 0 && (bit & (RANK_1 | RANK_2)) == 0) attacks |= bit >> 17;  // one left, two down

        table[square] = attacks;
    }
    return table;
}

inline constexpr auto WHITE_PAWN_ATTACKS = makeWhitePawnAttacks();
inline constexpr auto BLACK_PAWN_ATTACKS = makeBlackPawnAttacks();
inline constexpr auto KNIGHT_ATTACKS = makeKnightAttacks();