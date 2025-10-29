#pragma once

#include <array>
#include <cstdint>
#include <stack>
#include <string>
#include <string_view>
#include <type_traits>
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

struct CheckInfo {
    std::uint64_t checkers = 0;
    std::uint64_t block_mask = 0ULL;
    std::uint64_t pinned = 0;
    std::uint64_t king_unsafe = 0;
    std::uint64_t pin_dirs[64];
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
    bool white_castle_kingside = true;
    bool white_castle_queenside = true;
    bool black_castle_kingside = true;
    bool black_castle_queenside = true;

    static Board initFEN(std::string_view fen);
    std::string toString() const;
 
    std::uint64_t currentOccupied() const; // Returns bitboard for color of current turn
    std::uint64_t nextOccupied() const; // Returns bitboard for color of next turn

    void pawnMoves(std::vector<Move>& out) const;
    void knightMoves(std::vector<Move>& out) const;
    void bishopMoves(std::vector<Move>& out) const;
    void rookMoves(std::vector<Move>& out) const;
    void queenMoves(std::vector<Move>& out) const;
    void kingMoves(std::vector<Move>& out) const;
    void generatePseudoMoves(std::vector<Move>& out) const;

    CheckInfo analyzeChecks() const;
    void generateLegalMoves(std::vector<Move>& out);

    void makeMove(const Move& move, std::stack<Board>& undo_stack);
    void unmakeMove(std::stack<Board>& undo_stack);

    int evaluate() const;

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

template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, int> = 0>
constexpr auto constAbs(const T& x) noexcept {
    return x < 0 ? -x : x;
}

constexpr std::array<std::uint64_t, 64> makeRayTable(std::initializer_list<int> deltas) {
    std::array<std::uint64_t, 64> table{};
    for (int sq = 0; sq < 64; ++sq) {
        std::uint64_t mask = 0;
        for (int delta : deltas) {
            int current = sq;
            while (true) {
                const int next = current + delta;
                if (next < 0 || next >= 64) break;

                const int curRank = current / 8;
                const int curFile = current % 8;
                const int nextRank = next / 8;
                const int nextFile = next % 8;

                if (constAbs(nextRank - curRank) > 1 || constAbs(nextFile - curFile) > 1) break;

                mask |= 1ULL << next;
                current = next;
            }
        }
        table[static_cast<std::size_t>(sq)] = mask;
    }
    return table;
}

inline constexpr auto ROOK_RAYS = makeRayTable({8, -8, 1, -1});
inline constexpr auto BISHOP_RAYS = makeRayTable({9, 7, -7, -9});