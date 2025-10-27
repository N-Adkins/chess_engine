#pragma once

#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <random>
#include <vector>

struct MagicData {
    std::uint64_t mask{};
    std::uint64_t magic{};
    std::uint32_t offset{};
    std::uint8_t index_bits{};
};

inline std::size_t magicIndex(const MagicData& m, std::uint64_t occupancy) {
    const std::uint64_t blockers = occupancy & m.mask;
    const std::uint64_t hash = blockers * m.magic;
    const std::uint64_t index = (hash >> (64 - m.index_bits));
    return static_cast<std::size_t>(m.offset + index);
}

// Internal helpers
namespace magic_detail {

inline constexpr std::uint64_t FILE_A = 0x0101010101010101ULL;
inline constexpr std::uint64_t FILE_H = 0x8080808080808080ULL;
inline constexpr std::uint64_t RANK_1 = 0x00000000000000FFULL;
inline constexpr std::uint64_t RANK_8 = 0xFF00000000000000ULL;

inline int fileOf(int sq) { return sq & 7; }
inline int rankOf(int sq) { return sq >> 3; }

inline std::uint64_t setBit(int sq) { return 1ULL << sq; }

inline std::uint64_t rookMask(int sq) {
    std::uint64_t m = 0;
    int r = rankOf(sq), f = fileOf(sq);
    // Up (exclude edge)
    for (int rr = r + 1; rr <= 6; ++rr) m |= setBit(rr * 8 + f);
    // Down
    for (int rr = r - 1; rr >= 1; --rr) m |= setBit(rr * 8 + f);
    // Right
    for (int ff = f + 1; ff <= 6; ++ff) m |= setBit(r * 8 + ff);
    // Left
    for (int ff = f - 1; ff >= 1; --ff) m |= setBit(r * 8 + ff);
    return m;
}

inline std::uint64_t bishopMask(int sq) {
    std::uint64_t m = 0;
    int r = rankOf(sq), f = fileOf(sq);
    // NE
    for (int rr = r + 1, ff = f + 1; rr <= 6 && ff <= 6; ++rr, ++ff) m |= setBit(rr * 8 + ff);
    // NW
    for (int rr = r + 1, ff = f - 1; rr <= 6 && ff >= 1; ++rr, --ff) m |= setBit(rr * 8 + ff);
    // SE
    for (int rr = r - 1, ff = f + 1; rr >= 1 && ff <= 6; --rr, ++ff) m |= setBit(rr * 8 + ff);
    // SW
    for (int rr = r - 1, ff = f - 1; rr >= 1 && ff >= 1; --rr, --ff) m |= setBit(rr * 8 + ff);
    return m;
}

inline std::uint64_t rookAttacksSlow(int sq, std::uint64_t occ) {
    std::uint64_t a = 0;
    int r = rankOf(sq), f = fileOf(sq);
    // Up
    for (int rr = r + 1; rr <= 7; ++rr) { int s = rr * 8 + f; a |= setBit(s); if (occ & setBit(s)) break; }
    // Down
    for (int rr = r - 1; rr >= 0; --rr) { int s = rr * 8 + f; a |= setBit(s); if (occ & setBit(s)) break; }
    // Right
    for (int ff = f + 1; ff <= 7; ++ff) { int s = r * 8 + ff; a |= setBit(s); if (occ & setBit(s)) break; }
    // Left
    for (int ff = f - 1; ff >= 0; --ff) { int s = r * 8 + ff; a |= setBit(s); if (occ & setBit(s)) break; }
    return a;
}

inline std::uint64_t bishopAttacksSlow(int sq, std::uint64_t occ) {
    std::uint64_t a = 0;
    int r = rankOf(sq), f = fileOf(sq);
    // NE
    for (int rr = r + 1, ff = f + 1; rr <= 7 && ff <= 7; ++rr, ++ff) { int s = rr * 8 + ff; a |= setBit(s); if (occ & setBit(s)) break; }
    // NW
    for (int rr = r + 1, ff = f - 1; rr <= 7 && ff >= 0; ++rr, --ff) { int s = rr * 8 + ff; a |= setBit(s); if (occ & setBit(s)) break; }
    // SE
    for (int rr = r - 1, ff = f + 1; rr >= 0 && ff <= 7; --rr, ++ff) { int s = rr * 8 + ff; a |= setBit(s); if (occ & setBit(s)) break; }
    // SW
    for (int rr = r - 1, ff = f - 1; rr >= 0 && ff >= 0; --rr, --ff) { int s = rr * 8 + ff; a |= setBit(s); if (occ & setBit(s)) break; }
    return a;
}

inline int bitcount(std::uint64_t x) { return std::popcount(x); }
inline int lsb(std::uint64_t x) { return std::countr_zero(x); }

inline std::vector<std::uint64_t> enumerateSubsets(std::uint64_t mask) {
    // Build list of bit positions
    std::vector<int> bits;
    std::uint64_t m = mask;
    while (m) {
        int b = lsb(m);
        bits.push_back(b);
        m &= (m - 1);
    }
    const std::size_t n = bits.size();
    std::vector<std::uint64_t> out;
    out.reserve(1ULL << n);
    for (std::size_t i = 0; i < (1ULL << n); ++i) {
        std::uint64_t occ = 0;
        for (std::size_t j = 0; j < n; ++j) {
            if (i & (1ULL << j)) occ |= setBit(bits[j]);
        }
        out.push_back(occ);
    }
    return out;
}

inline std::uint64_t randomU64(std::mt19937_64& rng) {
    return rng();
}

// Sparse magic candidate (improves success rate)
inline std::uint64_t randomMagic(std::mt19937_64& rng) {
    return randomU64(rng) & randomU64(rng) & randomU64(rng);
}

struct BuiltMagic {
    std::array<MagicData, 64> data{};
    std::vector<std::uint64_t> table; // concatenated attacks
};

template <typename AttackFn>
inline BuiltMagic buildSliderMagics(AttackFn attacks_slow, bool rook_like) {
    BuiltMagic bm;
    bm.table.clear();
    bm.table.reserve(1 << 20); // pre-reserve some

    std::seed_seq seed{0x9e, 0x37, 0x79, 0xb9, 0x7f, 0x4a, 0x7c, 0x15};
    std::mt19937_64 rng(seed);

    for (int sq = 0; sq < 64; ++sq) {
        const std::uint64_t mask = rook_like ? rookMask(sq) : bishopMask(sq);
        const int bits = bitcount(mask);
        const std::size_t table_size = 1ULL << bits;

        // Enumerate all blocker subsets and their attacks
        const auto occs = enumerateSubsets(mask);
        std::vector<std::uint64_t> attacks(occs.size());
        for (std::size_t i = 0; i < occs.size(); ++i) {
            attacks[i] = attacks_slow(sq, occs[i]);
        }

        // Search magic
        std::vector<std::uint64_t> tmp(table_size);
        std::uint64_t magic = 0;
        bool found = false;

        while(!found) {
            magic = randomMagic(rng);

            // Reject bad upper bits (optional heuristic)
            if (bitcount((mask * magic) & 0xFF00000000000000ULL) < 6) continue;

            std::fill(tmp.begin(), tmp.end(), 0xFFFFFFFFFFFFFFFFULL);
            bool ok = true;

            for (std::size_t i = 0; i < occs.size(); ++i) {
                const std::uint64_t blockers = occs[i];
                const std::uint64_t hash = blockers * magic;
                const std::size_t index = (hash >> (64 - bits)) & (table_size - 1);
                const std::uint64_t atk = attacks[i];

                if (tmp[index] == 0xFFFFFFFFFFFFFFFFULL) {
                    tmp[index] = atk;
                } else if (tmp[index] != atk) {
                    ok = false;
                    break;
                }
            }

            if (ok) found = true;
        }

        MagicData md{};
        md.mask = mask;
        md.magic = magic;
        md.index_bits = static_cast<std::uint8_t>(bits);
        md.offset = static_cast<std::uint32_t>(bm.table.size());

        bm.data[sq] = md;
        // Append tmp table
        bm.table.insert(bm.table.end(), tmp.begin(), tmp.end());
    }

    return bm;
}

} // namespace magic_detail

// Global magic tables (header-only, so mark inline)
inline std::array<MagicData, 64> ROOK_MAGIC_DATA{};
inline std::array<MagicData, 64> BISHOP_MAGIC_DATA{};
inline std::vector<std::uint64_t> SLIDER_ATTACKS;

// Initialize magic tables (call once at startup)
inline void initMagic() {
    using namespace magic_detail;
    // Build rook
    auto rook = buildSliderMagics(rookAttacksSlow, true);
    ROOK_MAGIC_DATA = rook.data;

    // Build bishop
    auto bishop = buildSliderMagics(bishopAttacksSlow, false);
    BISHOP_MAGIC_DATA = bishop.data;

    // Combine tables: rook first, then bishop with adjusted offsets
    SLIDER_ATTACKS.clear();
    SLIDER_ATTACKS.reserve(rook.table.size() + bishop.table.size());

    // Rook
    SLIDER_ATTACKS.insert(SLIDER_ATTACKS.end(), rook.table.begin(), rook.table.end());

    // Adjust bishop offsets and append
    const std::uint32_t base = static_cast<std::uint32_t>(SLIDER_ATTACKS.size());
    for (int sq = 0; sq < 64; ++sq) {
        BISHOP_MAGIC_DATA[sq].offset += base;
    }
    SLIDER_ATTACKS.insert(SLIDER_ATTACKS.end(), bishop.table.begin(), bishop.table.end());
}

// Lookup functions
inline std::uint64_t rookAttacks(int square, std::uint64_t occupancy) {
    const auto& m = ROOK_MAGIC_DATA[square];
    return SLIDER_ATTACKS[magicIndex(m, occupancy)];
}

inline std::uint64_t bishopAttacks(int square, std::uint64_t occupancy) {
    const auto& m = BISHOP_MAGIC_DATA[square];
    return SLIDER_ATTACKS[magicIndex(m, occupancy)];
}

inline std::uint64_t queenAttacks(int square, std::uint64_t occupancy) {
    return rookAttacks(square, occupancy) | bishopAttacks(square, occupancy);
}