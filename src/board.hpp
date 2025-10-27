#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

constexpr std::string_view STARTING_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

struct Move {
    int from, to;
    
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

    std::uint64_t bishopAttacks(int square, std::uint64_t occupancy) const;
    std::uint64_t rookAttacks(int square, std::uint64_t occupancy) const;
    std::uint64_t queenAttacks(int square, std::uint64_t occupancy) const;

private:
    Board() = default;
};

constexpr std::uint64_t FILE_A = 0x0101010101010101ULL;
constexpr std::uint64_t FILE_H = 0x8080808080808080ULL;

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

        if ((bit & ~(FILE_A | (FILE_A << 8))) != 0) attacks |= bit << 15; // two left, one up
        if ((bit & ~(FILE_H | (FILE_H << 8))) != 0) attacks |= bit << 17; // two right, one up
        if ((bit & ~(FILE_A | (FILE_A >> 8))) != 0) attacks |= bit >> 17; // two left, one down
        if ((bit & ~(FILE_H | (FILE_H >> 8))) != 0) attacks |= bit >> 15; // two right, one down

        if ((bit & ~(FILE_A | (FILE_A << 16))) != 0) attacks |= bit << 6;  // one left, two up
        if ((bit & ~(FILE_H | (FILE_H << 16))) != 0) attacks |= bit << 10; // one right, two up
        if ((bit & ~(FILE_A | (FILE_A >> 16))) != 0) attacks |= bit >> 10; // one left, two down
        if ((bit & ~(FILE_H | (FILE_H >> 16))) != 0) attacks |= bit >> 6;  // one right, two down

        table[square] = attacks;
    }
    return table;
}

inline constexpr auto WHITE_PAWN_ATTACKS = makeWhitePawnAttacks();
inline constexpr auto BLACK_PAWN_ATTACKS = makeBlackPawnAttacks();
inline constexpr auto KNIGHT_ATTACKS = makeKnightAttacks();