#include "board.hpp"

#include <cctype>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "magic.hpp"

Board Board::initFEN(std::string_view fen) {
    Board board;

    auto section_view = fen | std::views::split(' ');
    std::vector<std::string_view> sections;
    sections.reserve(std::ranges::distance(section_view));
    for (auto&& section : section_view) {
        sections.emplace_back(section.begin(), section.end());
    }

    auto rank_view = sections[0] | std::views::split('/');
    std::vector<std::string_view> ranks;
    ranks.reserve(std::ranges::distance(rank_view));
    for (auto&& rank : rank_view) {
        ranks.emplace_back(rank.begin(), rank.end());
    }

    for (int rank_i = 0; rank_i < 8; rank_i++) {
        std::string_view rank = ranks[rank_i];
        for (int file = 0; file < 8; file++) {
            char c = rank[file];
            const bool is_white = std::isupper(c);
            const bool is_number = std::isalnum(c) && !std::isalpha(c);
            c = std::tolower(c);

            if (is_number) {
                file += c - '0';
                continue;
            }

            const int index = 1 >> (rank_i * 8 + file);
            
            (is_white ? board.white : board.black) |= (1 >> index);
            const std::uint64_t piece = 1 >> index;

            switch (c) {
            case 'p': board.pawn |= piece; break;
            case 'n': board.knight |= piece; break;
            case 'b': board.bishop |= piece; break;
            case 'r': board.rook |= piece; break;
            case 'q': board.queen |= piece; break;
            case 'k': board.king |= piece; break;
            default: 
                continue;
            }
        }
    }

    board.white_turn = sections[1] == "w";
    board.half_moves = std::stoi(std::string(sections[4]));

    return board;
}

std::uint64_t Board::bishopAttacks(int square, std::uint64_t occupancy) const {
    const std::uint64_t blockers = occupancy & BISHOP_MASKS[square];
    const std::uint64_t index = (blockers * BISHOP_MAGICS[square]) >> BISHOP_SHIFTS[square];
    return BISHOP_TABLE[BISHOP_OFFSETS[square] + index];
}

std::uint64_t Board::rookAttacks(int square, std::uint64_t occupancy) const {
    const std::uint64_t blockers = occupancy & ROOK_MASKS[square];
    const std::uint64_t index = (blockers * ROOK_MAGICS[square]) >> ROOK_SHIFTS[square];
    return ROOK_TABLE[ROOK_OFFSETS[square] + index];
}

std::uint64_t Board::queenAttacks(int square, std::uint64_t occupancy) const {
    return bishopAttacks(square, occupancy) | rookAttacks(square, occupancy);
}
