#include "board.hpp"

#include <bit>
#include <cctype>
#include <cstdint>
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

// Helpers for attack generation and geometry
static inline int kingSquare(const Board& b, bool white_side) {
    std::uint64_t k = b.king & (white_side ? b.white : b.black);
    return std::countr_zero(k);
}

static inline bool sameLineOrDiag(int a, int b) {
    int ar = a / 8, af = a % 8;
    int br = b / 8, bf = b % 8;
    return (ar == br) || (af == bf) || (std::abs(ar - br) == std::abs(af - bf));
}

static inline std::uint64_t betweenMask(int a, int b) {
    if (a == b) return 0ULL;
    if (!sameLineOrDiag(a, b)) return 0ULL;
    int ar = a / 8, af = a % 8;
    int br = b / 8, bf = b % 8;
    int dr = (br > ar) ? 1 : (br < ar ? -1 : 0);
    int df = (bf > af) ? 1 : (bf < af ? -1 : 0);
    std::uint64_t mask = 0ULL;
    int r = ar + dr, f = af + df;
    while (r != br || f != bf) {
        mask |= 1ULL << (r * 8 + f);
        r += dr; f += df;
    }
    // Exclude endpoints
    mask &= ~(1ULL << a);
    mask &= ~(1ULL << b);
    return mask;
}

static inline std::uint64_t knightAttacksFrom(std::uint64_t knights) {
    std::uint64_t attacks = 0ULL;
    std::uint64_t k = knights;
    while (k) {
        int sq = popLSB(k);
        attacks |= KNIGHT_ATTACKS[sq];
    }
    return attacks;
}

static inline std::uint64_t pawnAttacksFrom(std::uint64_t pawns, bool white_pawns) {
    std::uint64_t attacks = 0ULL;
    std::uint64_t p = pawns;
    while (p) {
        int sq = popLSB(p);
        attacks |= white_pawns ? WHITE_PAWN_ATTACKS[sq] : BLACK_PAWN_ATTACKS[sq];
    }
    return attacks;
}

static inline std::uint64_t bishopAttacksFrom(std::uint64_t bishops, std::uint64_t occ) {
    std::uint64_t attacks = 0ULL;
    std::uint64_t b = bishops;
    while (b) {
        int sq = popLSB(b);
        attacks |= bishopAttacks(sq, occ);
    }
    return attacks;
}

static inline std::uint64_t rookAttacksFrom(std::uint64_t rooks, std::uint64_t occ) {
    std::uint64_t attacks = 0ULL;
    std::uint64_t r = rooks;
    while (r) {
        int sq = popLSB(r);
        attacks |= rookAttacks(sq, occ);
    }
    return attacks;
}

static inline std::uint64_t kingAttacksFrom(int sq) {
    const std::uint64_t bb = 1ULL << sq;
    const std::uint64_t notA = ~FILE_A;
    const std::uint64_t notH = ~FILE_H;
    std::uint64_t attacks = 0ULL;
    attacks |= (bb << 8) | (bb >> 8);
    attacks |= (bb & notH) << 1;
    attacks |= (bb & notA) >> 1;
    attacks |= (bb & notH) << 9;
    attacks |= (bb & notH) >> 7;
    attacks |= (bb & notA) << 7;
    attacks |= (bb & notA) >> 9;
    return attacks;
}

// Determine if square 'sq' is attacked by the given side (byWhite=true => white attacks)
static inline bool squareAttacked(int sq, bool byWhite, const Board& b, std::uint64_t occ) {
    const std::uint64_t pawns = b.pawn & (byWhite ? b.white : b.black);
    const std::uint64_t knights = b.knight & (byWhite ? b.white : b.black);
    const std::uint64_t bishops = b.bishop & (byWhite ? b.white : b.black);
    const std::uint64_t rooks = b.rook & (byWhite ? b.white : b.black);
    const std::uint64_t queens = b.queen & (byWhite ? b.white : b.black);
    const std::uint64_t kBB = b.king & (byWhite ? b.white : b.black);

    // Pawns: reverse lookup via opponent's tables
    if (byWhite) {
        if (BLACK_PAWN_ATTACKS[sq] & pawns) return true;
    } else {
        if (WHITE_PAWN_ATTACKS[sq] & pawns) return true;
    }
    // Knights
    if (KNIGHT_ATTACKS[sq] & knights) return true;
    // Bishops/Queens diagonals
    if (bishopAttacks(sq, occ) & (bishops | queens)) return true;
    // Rooks/Queens orthogonals
    if (rookAttacks(sq, occ) & (rooks | queens)) return true;
    // King
    if (kBB && (kingAttacksFrom(sq) & kBB)) return true;
    return false;
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

    for (int rank_i = 0; rank_i < 8; rank_i++) {
        std::string_view rank = ranks[rank_i];
        int file = 0;
        for (std::size_t i = 0; i < rank.size(); ++i) {
            char c = rank[i];
            if (std::isdigit(static_cast<unsigned char>(c))) {
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
    }

    return output;
}

std::uint64_t Board::currentOccupied() const {
    return white_turn ? white : black;
}

std::uint64_t Board::nextOccupied() const {
    return white_turn ? black : white; 
}

void Board::pawnMoves(std::vector<Move>& out) const {
    const std::uint64_t current = currentOccupied();
    const std::uint64_t next = nextOccupied();
    const std::uint64_t occ = white | black;
    
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
                if (occ & (1ULL << to)) continue;
                out.push_back(Move {
                    .from = from,
                    .to = to,
                    .flags = 0,
                });

                if (from <= 15 && from > 7) {
                    const int double_to = from + 16;
                    if (occ & (1ULL << double_to)) continue;
                    out.push_back(Move {
                        .from = from,
                        .to = double_to,
                        .flags = static_cast<std::uint8_t>(MoveFlag::DoublePush),
                    });
                }
            }
        } else {
            const bool IS_LAST_RANK = from <= 7;
            if (!IS_LAST_RANK) {
                const int to = from - 8;
                if (occ & (1ULL << to)) continue;
                out.push_back(Move {
                    .from = from,
                    .to = to,
                    .flags = 0,
                });

                if (from <= 55 && from > 47) {
                    const int double_to = from - 16;
                    if (occ & (1ULL << double_to)) continue;
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

void Board::knightMoves(std::vector<Move>& out) const {
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

void Board::bishopMoves(std::vector<Move>& out) const {
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

void Board::rookMoves(std::vector<Move>& out) const {
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

void Board::queenMoves(std::vector<Move>& out) const {
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

void Board::kingMoves(std::vector<Move>& out) const {
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

void Board::generatePseudoMoves(std::vector<Move>& out) const {
    pawnMoves(out);
    knightMoves(out);
    bishopMoves(out);
    rookMoves(out);
    queenMoves(out);
    kingMoves(out);
}

CheckInfo Board::analyzeChecks() const {
    CheckInfo info;
    for (int i = 0; i < 64; ++i) info.pin_dirs[i] = 0ULL;
    const std::uint64_t next = white_turn ? black : white;
    const std::uint64_t occ = white | black;

    // King square
    const int ksq = kingSquare(*this, white_turn);

    // Enemy piece sets
    const std::uint64_t enemyPawns = pawn & next;
    const std::uint64_t enemyKnights = knight & next;
    const std::uint64_t enemyBishops = bishop & next;
    const std::uint64_t enemyRooks = rook & next;
    const std::uint64_t enemyQueens = queen & next;
    const int enemyKingSq = kingSquare(*this, !white_turn);

    // 1) Checkers detection
    // Knights
    info.checkers |= KNIGHT_ATTACKS[ksq] & enemyKnights;
    // Pawns
    if (white_turn) {
        // Black attacks downwards (from their perspective)
        info.checkers |= BLACK_PAWN_ATTACKS[ksq] & enemyPawns;
    } else {
        info.checkers |= WHITE_PAWN_ATTACKS[ksq] & enemyPawns;
    }
    // Sliding
    const std::uint64_t bishopsLike = enemyBishops | enemyQueens;
    const std::uint64_t rooksLike = enemyRooks | enemyQueens;
    info.checkers |= bishopAttacks(ksq, occ) & bishopsLike;
    info.checkers |= rookAttacks(ksq, occ) & rooksLike;
    // Adjacent enemy king cannot "check" in a legal position but mark as unsafe

    // 2) Block mask if in check
    const int numCheckers = std::popcount(info.checkers);
    if (numCheckers == 1) {
        const int checkerSq = std::countr_zero(info.checkers);
        const std::uint64_t checkerBB = 1ULL << checkerSq;
        bool slidingChecker = ((checkerBB & (bishopsLike | rooksLike)) != 0) && sameLineOrDiag(ksq, checkerSq);
        if (slidingChecker) {
            info.block_mask = betweenMask(ksq, checkerSq) | checkerBB;
        } else {
            // Knight/Pawn/King: only capture square resolves
            info.block_mask = checkerBB;
        }
    } else if (numCheckers >= 2) {
        info.block_mask = 0ULL; // Only king moves allowed
    } else {
        info.block_mask = 0ULL; // Not in check
    }

    // 3) Pinned pieces detection and pin direction masks
    // Scan 8 directions from king; if exactly one friendly piece then an enemy slider, mark that piece pinned
    auto markPin = [&](int dr, int df, bool rook_dir) {
        int r = ksq / 8 + dr;
        int f = ksq % 8 + df;
        int firstSq = -1;
        while (r >= 0 && r < 8 && f >= 0 && f < 8) {
            int sq = r * 8 + f;
            std::uint64_t bb = 1ULL << sq;
            if (occ & bb) {
                if (firstSq == -1) {
                    // First piece we see
                    firstSq = sq;
                    // If it's enemy, this direction can't pin
                    if (bb & next) return;
                } else {
                    // Second piece we see: check if it's appropriate enemy slider
                    bool enemyIsRookLike = (bb & (rooksLike)) != 0;
                    bool enemyIsBishopLike = (bb & (bishopsLike)) != 0;
                    bool ok = rook_dir ? enemyIsRookLike : enemyIsBishopLike;
                    if (ok) {
                        // Pin detected: firstSq is friendly pinned piece
                        info.pinned |= (1ULL << firstSq);
                        // Build pin direction mask: squares along the line from king to enemy inclusive
                        std::uint64_t ray = 0ULL;
                        int rr = ksq / 8 + dr;
                        int ff = ksq % 8 + df;
                        while (rr >= 0 && rr < 8 && ff >= 0 && ff < 8) {
                            int s = rr * 8 + ff;
                            ray |= (1ULL << s);
                            if (s == sq) break;
                            rr += dr; ff += df;
                        }
                        info.pin_dirs[firstSq] = ray | (1ULL << ksq);
                    }
                    return;
                }
            }
            r += dr; f += df;
        }
    };

    // Orthogonal directions
    markPin(1, 0, true);
    markPin(-1, 0, true);
    markPin(0, 1, true);
    markPin(0, -1, true);
    // Diagonals
    markPin(1, 1, false);
    markPin(1, -1, false);
    markPin(-1, 1, false);
    markPin(-1, -1, false);

    // 4) Enemy attack map (for king safety)
    std::uint64_t enemyAttacks = 0ULL;
    enemyAttacks |= pawnAttacksFrom(enemyPawns, !white_turn);
    enemyAttacks |= knightAttacksFrom(enemyKnights);
    enemyAttacks |= bishopAttacksFrom(enemyBishops | enemyQueens, occ);
    enemyAttacks |= rookAttacksFrom(enemyRooks | enemyQueens, occ);
    enemyAttacks |= kingAttacksFrom(enemyKingSq);
    info.king_unsafe = enemyAttacks;

    return info;
}

void Board::generateLegalMoves(std::vector<Move>& out) {
    out.clear();
    std::vector<Move> pseudo;
    generatePseudoMoves(pseudo);

    const auto check_info = analyzeChecks();
    const int checks = std::popcount(check_info.checkers);
    const std::uint64_t occ = white | black;
    const std::uint64_t current_bb = currentOccupied();
    const std::uint64_t next_bb = nextOccupied();

    for (const auto& mv : pseudo) {
        const std::uint64_t from_bb = 1ULL << mv.from;
        const std::uint64_t to_bb = 1ULL << mv.to;
        const bool movingKing = ((king & current_bb) & from_bb) != 0;

        if (movingKing) {
            // King cannot move into attacked square; account for captures by removing target from occ
            std::uint64_t occ_for = occ & ~from_bb;
            if (to_bb & next_bb) occ_for &= ~to_bb;
            if (squareAttacked(mv.to, !white_turn, *this, occ_for)) continue;
            out.push_back(mv);
            continue;
        }

        // If double check, only king moves allowed
        if (checks >= 2) continue;

        // Pinned piece constraint: must move along pin ray
        if (check_info.pinned & from_bb) {
            if ((check_info.pin_dirs[mv.from] & to_bb) == 0) continue;
        }

        // If in single check, non-king moves must capture the checker or block the line
        if (checks == 1) {
            if ((check_info.block_mask & to_bb) == 0) continue;
        }

        out.push_back(mv);
    }
}

void Board::makeMove(const Move& move, std::stack<Board>& undo_stack) {
    undo_stack.push(*this);

    const std::uint64_t from_board = 1ULL << move.from;
    const std::uint64_t to_board = 1ULL << move.to;
    const std::uint64_t move_board = from_board | to_board;

    std::uint64_t* mover = nullptr;
    if (pawn & from_board) {
        mover = &pawn;
    } else if (knight & from_board) {
        mover = &knight;
    } else if (bishop & from_board) {
        mover = &bishop;
    } else if (rook & from_board) {
        mover = &rook;
    } else if (queen & from_board) {
        mover = &queen;
    } else if (king & from_board) {
        mover = &king;
    }
    assert(mover != nullptr);

    *mover ^= move_board;
    (white_turn ? white : black) ^= move_board;

    if (move.flags & static_cast<std::uint8_t>(MoveFlag::Capture)) {
        std::uint64_t& enemy = white_turn ? black : white;
        enemy &= ~to_board;
        pawn &= ~to_board;
        knight &= ~to_board;
        bishop &= ~to_board;
        rook &= ~to_board;
        queen &= ~to_board;
        king &= ~to_board;
    }

    *mover &= ~from_board;
    *mover |= to_board;

    if (white_turn) {
        white &= ~from_board;
        white |= to_board;
    } else {
        black &= ~from_board;
        black |= to_board;
    }

    white_turn = !white_turn;
}

void Board::unmakeMove(std::stack<Board>& undo_stack) {
    *this = undo_stack.top();
    undo_stack.pop();
}