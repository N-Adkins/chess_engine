#include "board.hpp"

#include <bit>
#include <cctype>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "magic.hpp"

static int popLSB(std::uint64_t& bitboard) {
    int count = std::countr_zero(bitboard);
    bitboard &= bitboard - 1;
    return count;
}

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

    // FEN ranks come from rank 8 (top) to rank 1 (bottom). We'll map
    // them to bitboard squares where square 0 == a1 and square 63 == h8.
    for (int rank_i = 0; rank_i < 8; rank_i++) {
        std::string_view rank = ranks[rank_i];
        int file = 0;
        for (std::size_t i = 0; i < rank.size(); ++i) {
            char c = rank[i];
            if (std::isdigit(static_cast<unsigned char>(c))) {
                // move file forward by the digit count
                file += (c - '0');
                continue;
            }

            const bool is_white = std::isupper(static_cast<unsigned char>(c));
            const char lc = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            if (file < 0 || file >= 8) continue; // defensive

            const int square = (7 - rank_i) * 8 + file;
            const std::uint64_t bit = 1ULL << square;

            if (is_white) board.white |= bit; else board.black |= bit;

            switch (lc) {
            case 'p': board.pawn |= bit; break;
            case 'n': board.knight |= bit; break;
            case 'b': board.bishop |= bit; break;
            case 'r': board.rook |= bit; break;
            case 'q': board.queen |= bit; break;
            case 'k': board.king |= bit; break;
            default:
                break;
            }

            ++file;
        }
    }

    board.white_turn = sections[1] == "w";
    board.half_moves = std::stoi(std::string(sections[4]));

    return board;
}

std::string Board::toString() const {
    std::string output;
    output.reserve(8 * 9);
    for (int rank = 7; rank >= 0; --rank) {
        for (int file = 0; file < 8; ++file) {
            const int square = rank * 8 + file;
            const std::uint64_t mask = 1ULL << square;
            char ch = '-';
            if ((pawn & mask) != 0) ch = 'p';
            else if ((knight & mask) != 0) ch = 'n';
            else if ((bishop & mask) != 0) ch = 'b';
            else if ((rook & mask) != 0) ch = 'r';
            else if ((queen & mask) != 0) ch = 'q';
            else if ((king & mask) != 0) ch = 'k';

            if ((white & mask) != 0 && ch != '-') ch = static_cast<char>(std::toupper(ch));
            output.push_back(ch);
        }
        if (rank != 0) output.push_back('\n');
    }

    return output;
}

std::uint64_t Board::currentOccupied() const {
    return white_turn ? white : black;
}

std::uint64_t Board::nextOccupied() const {
    return white_turn ? black : white; 
}

void Board::pawnMoves(std::vector<Move>& out) {
    const std::uint64_t current = currentOccupied();
    const std::uint64_t next = nextOccupied();
    
    std::uint64_t current_pawns = current & pawn;
    while (current_pawns) {
        const int from = popLSB(current_pawns);
        std::uint64_t attacks = next & (white_turn ? WHITE_PAWN_ATTACKS[from] : BLACK_PAWN_ATTACKS[from]);
        while (attacks) {
            const int to = popLSB(attacks);
            out.push_back(Move{
                .from = from,
                .to = to,
                .flags = static_cast<std::uint8_t>(MoveFlag::Capture),
            });
        }

        if (white_turn) {
            const bool IS_LAST_RANK = from >= 56;
            if (!IS_LAST_RANK) {
                const int to = from + 8;
                out.push_back(Move {
                    .from = from,
                    .to = to,
                    .flags = 0,
                });

                if (from <= 15 && from > 7) {
                    const int double_to = from + 16;
                    out.push_back(Move {
                        .from = from,
                        .to = double_to,
                        .flags = static_cast<std::uint8_t>(MoveFlag::DoublePush),
                    });
                }
            }
        } else {
            const bool IS_LAST_RANK = from <= 8;
            if (!IS_LAST_RANK) {
                const int to = from - 8;
                out.push_back(Move {
                    .from = from,
                    .to = to,
                    .flags = 0,
                });

                if (from <= 55 && from > 47) {
                    const int double_to = from - 16;
                    if (next & (1 << double_to)) continue;
                    out.push_back(Move {
                        .from = from,
                        .to = double_to,
                        .flags = static_cast<std::uint8_t>(MoveFlag::DoublePush),
                    });
                }
            }
        }
    }

}

void Board::knightMoves(std::vector<Move>& out) {
    const std::uint64_t current = currentOccupied();
    const std::uint64_t next = nextOccupied();
    std::uint64_t current_knights = knight & current;
    while (current_knights) {
        const int from = popLSB(current_knights);
        std::uint64_t moves = KNIGHT_ATTACKS[from] & ~current;
        while (moves) {
            const int to = popLSB(moves);
            const bool capture = ((1ULL << to) & next) != 0;
            out.push_back(Move {
                .from = from,
                .to = to,
                .flags = capture ? static_cast<std::uint8_t>(MoveFlag::Capture) : static_cast<std::uint8_t>(0),
            });
        }
    }
}

void Board::bishopMoves(std::vector<Move>& out) {
    const std::uint64_t current = currentOccupied();
    const std::uint64_t next = nextOccupied();
    std::uint64_t bishops = bishop & current;
    while (bishops) {
        const int from = popLSB(bishops);
        std::uint64_t moves = bishopAttacks(from, (white | black) & ~(1ULL << from)) & ~current;
        while (moves) {
            const int to = popLSB(moves);
            bool capture = (next & (1ULL << to)) != 0;
            out.push_back(Move {
                .from = from,
                .to = to,
                .flags = capture ? static_cast<std::uint8_t>(MoveFlag::Capture) : static_cast<std::uint8_t>(0),
            });
        }
    }
}

void Board::rookMoves(std::vector<Move>& out) {
    const std::uint64_t current = currentOccupied();
    const std::uint64_t next = nextOccupied();
    std::uint64_t rooks = rook & current;
    while (rooks) {
        const int from = popLSB(rooks);
        std::uint64_t moves = rookAttacks(from, (white | black) & ~(1ULL << from)) & ~current;
        while (moves) {
            const int to = popLSB(moves);
            bool capture = (next & (1ULL << to)) != 0;
            out.push_back(Move {
                .from = from,
                .to = to,
                .flags = capture ? static_cast<std::uint8_t>(MoveFlag::Capture) : static_cast<std::uint8_t>(0),
            });
        }
    }
}

void Board::queenMoves(std::vector<Move>& out) {
    const std::uint64_t current = currentOccupied();
    const std::uint64_t next = nextOccupied();
    std::uint64_t queens = queen & current;
    while (queens) {
        const int from = popLSB(queens);
        const std::uint64_t occupancy = (white | black) & ~(1ULL << from);
        std::uint64_t moves = queenAttacks(from, occupancy) & ~current;
        while (moves) {
            const int to = popLSB(moves);
            bool capture = (next & (1ULL << to)) != 0;
            out.push_back(Move {
                .from = from,
                .to = to,
                .flags = capture ? static_cast<std::uint8_t>(MoveFlag::Capture) : static_cast<std::uint8_t>(0),
            });
        }
    }
}

void Board::kingMoves(std::vector<Move>& out) {
    const std::uint64_t current = currentOccupied();
    const std::uint64_t next = nextOccupied();

    std::uint64_t kings = king & current;
    while (kings) {
        const int from = popLSB(kings);
        const std::uint64_t bb = 1ULL << from;

        const std::uint64_t notA = ~FILE_A;
        const std::uint64_t notH = ~FILE_H;

        std::uint64_t attacks = 0;
        attacks |= (bb << 8) | (bb >> 8);          // N, S
        attacks |= (bb & notH) << 1;               // E
        attacks |= (bb & notA) >> 1;               // W
        attacks |= (bb & notH) << 9;               // NE
        attacks |= (bb & notH) >> 7;               // SE
        attacks |= (bb & notA) << 7;               // NW
        attacks |= (bb & notA) >> 9;               // SW

        std::uint64_t moves = attacks & ~current;
        while (moves) {
            const int to = popLSB(moves);
            const bool capture = (next & (1ULL << to)) != 0;
            out.push_back(Move{
                .from = from,
                .to = to,
                .flags = capture ? static_cast<std::uint8_t>(MoveFlag::Capture) : static_cast<std::uint8_t>(0),
            });
        }
    }
}

void Board::generatePseudoMoves(std::vector<Move>& out) {
    pawnMoves(out);
    knightMoves(out);
    bishopMoves(out);
    rookMoves(out);
    queenMoves(out);
    kingMoves(out);
}
