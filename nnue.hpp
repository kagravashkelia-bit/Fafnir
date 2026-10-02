/*
  SFNNv16 standalone evaluator - single-header implementation.

  This is an independent, dependency-free C++17 implementation of the
  SFNNv16 architecture used by the Stockfish 19 sf_19 tag.

  It does NOT include or require Stockfish sources. It owns:
    - FEN parsing
    - chess bitboards and attack generation
    - HalfKAv2_hm features
    - FullThreats features
    - PP_3Wide pawn-pair features
    - NNUE feature-transformer accumulators
    - SFNNv16 layer forward propagation
    - Stockfish NNUE LEB128 file loading
    - SF19 bucket selection
    - raw/internal and UCI-style cp output

  Compatibility target:
    Stockfish 19 / SFNNv16
    default SF19 net: nn-1a298aa575a0.nnue

  IMPORTANT LICENSE NOTE:
  SFNNv16's exact feature architecture and network file format are derived
  from the GPLv3 Stockfish 19 implementation. Treat this implementation as
  GPLv3-compatible when distributed with or derived from Stockfish-compatible
  code. See the Stockfish project for the authoritative license text.

  No Stockfish header or source file is required to compile this header.
*/

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <cmath>
#include <utility>
#include <type_traits>

namespace sfnnv16 {

// -----------------------------------------------------------------------------
// Scalar types / constants
// -----------------------------------------------------------------------------

using i8  = std::int8_t;
using u8  = std::uint8_t;
using i16 = std::int16_t;
using u16 = std::uint16_t;
using i32 = std::int32_t;
using u32 = std::uint32_t;
using i64 = std::int64_t;
using u64 = std::uint64_t;

static constexpr int L1 = 1024;
static constexpr int L2 = 32;
static constexpr int L3 = 32;
static constexpr int PSQTBuckets = 8;

static constexpr int WeightScaleBits = 6;
static constexpr int OutputScale = 16;
static constexpr int FtMaxVal = 255;
static constexpr int HiddenOneVal = 128;

static constexpr u32 FileVersion = 0x6A448AFAu;

// SF19 architecture hash. It is the hash stored in the NNUE file header.
static constexpr u32 FeatureTransformerHash = 0xCB685313u;
static constexpr u32 NetworkArchitectureHash = 0x63337116u;
static constexpr u32 ArchitectureHash = 0xA85B2205u;

// Feature dimensions.
static constexpr int HalfKADimensions = 22528; // 64 * (11 * 64) / 2
static constexpr int ThreatDimensions = 59808;
static constexpr int PairDimensions = 4560;
static constexpr int ThreatAndPairDimensions = ThreatDimensions + PairDimensions; // 64368
static constexpr int TransformerInputDimensions =
    HalfKADimensions + ThreatAndPairDimensions;

// -----------------------------------------------------------------------------
// Utility
// -----------------------------------------------------------------------------

inline std::int32_t arshift(i64 x, int n) {
    if (n <= 0) return static_cast<std::int32_t>(x);
    if (x >= 0) return static_cast<std::int32_t>(x >> n);
    // C++20 guarantees arithmetic shift, but this project targets C++17.
    return static_cast<std::int32_t>(-((-x) >> n));
}

inline i32 clamp_i32(i64 x, i32 lo, i32 hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return static_cast<i32>(x);
}

constexpr u32 rotl1(u32 x) noexcept {
    return (x << 1) | (x >> 31);
}

// -----------------------------------------------------------------------------
// Chess board
// square 0 = a1, ... 63 = h8.
// piece: 1..6 white, 9..14 black, exactly matching Stockfish's color^8
// convention for the feature indexing formulas.
// -----------------------------------------------------------------------------

enum Color : u8 { WHITE = 0, BLACK = 1 };

enum PieceType : u8 {
    NO_PIECE_TYPE = 0,
    PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6
};

enum Piece : u8 {
    NO_PIECE = 0,
    W_PAWN = 1, W_KNIGHT = 2, W_BISHOP = 3, W_ROOK = 4, W_QUEEN = 5, W_KING = 6,
    B_PAWN = 9, B_KNIGHT = 10, B_BISHOP = 11, B_ROOK = 12, B_QUEEN = 13, B_KING = 14
};

inline Color piece_color(Piece p) {
    return p == NO_PIECE ? WHITE : static_cast<Color>((static_cast<int>(p) >> 3) & 1);
}
inline PieceType piece_type(Piece p) {
    return p == NO_PIECE ? NO_PIECE_TYPE : static_cast<PieceType>(static_cast<int>(p) & 7);
}
inline Piece make_piece(Color c, PieceType pt) {
    return pt == NO_PIECE_TYPE ? NO_PIECE : static_cast<Piece>(static_cast<int>(pt) + 8 * static_cast<int>(c));
}

using Bitboard = u64;

inline Bitboard bb(int sq) { return 1ULL << sq; }
constexpr int file_of(int sq) { return sq & 7; }
constexpr int rank_of(int sq) { return sq >> 3; }

inline Bitboard file_bb(int f) { return 0x0101010101010101ULL << f; }
inline Bitboard rank_bb(int r) { return 0xFFULL << (8 * r); }

inline Bitboard shift_north(Bitboard b) { return b << 8; }
inline Bitboard shift_south(Bitboard b) { return b >> 8; }
inline Bitboard shift_east(Bitboard b) { return (b & ~0x8080808080808080ULL) << 1; }
inline Bitboard shift_west(Bitboard b) { return (b & ~0x0101010101010101ULL) >> 1; }
inline Bitboard shift_ne(Bitboard b) { return (b & ~0x8080808080808080ULL) << 9; }
inline Bitboard shift_nw(Bitboard b) { return (b & ~0x0101010101010101ULL) << 7; }
inline Bitboard shift_se(Bitboard b) { return (b & ~0x8080808080808080ULL) >> 7; }
inline Bitboard shift_sw(Bitboard b) { return (b & ~0x0101010101010101ULL) >> 9; }

inline int pop_lsb(Bitboard& b) {
#if defined(_MSC_VER)
    unsigned long idx;
#if defined(_WIN64)
    _BitScanForward64(&idx, b);
    b &= b - 1;
    return static_cast<int>(idx);
#else
    unsigned long lo = static_cast<unsigned long>(b & 0xffffffffu);
    if (lo) {
        _BitScanForward(&idx, lo);
        b &= b - 1;
        return static_cast<int>(idx);
    }
    _BitScanForward(&idx, static_cast<unsigned long>(b >> 32));
    b &= b - 1;
    return static_cast<int>(idx + 32);
#endif
#else
    int idx = __builtin_ctzll(b);
    b &= b - 1;
    return idx;
#endif
}

inline int popcount(Bitboard b) {
#if defined(_MSC_VER)
#  if defined(_WIN64)
    return static_cast<int>(__popcnt64(b));
#  else
    return static_cast<int>(__popcnt(static_cast<unsigned>(b)))
         + static_cast<int>(__popcnt(static_cast<unsigned>(b >> 32)));
#  endif
#else
    return __builtin_popcountll(b);
#endif
}

// Geometry of a pawn pair: own file + adjacent files, ranks 2..7,
// excluding the pawn itself.
inline Bitboard pawn_pair_bb(int s) {
    Bitboard f = file_bb(file_of(s));
    Bitboard files = f | shift_east(f) | shift_west(f);
    return files & ~(rank_bb(0) | rank_bb(7)) & ~bb(s);
}

// Pseudo attacks (ignore blockers).
inline Bitboard knight_attacks(int s) {
    const Bitboard k = bb(s);
    return shift_ne(shift_north(k) | shift_south(k))
         | shift_nw(shift_north(k) | shift_south(k))
         | shift_ne(shift_east(k) | shift_west(k))
         | shift_se(shift_east(k) | shift_west(k));
}

inline Bitboard king_attacks(int s) {
    Bitboard x = bb(s);
    return shift_north(x) | shift_south(x) | shift_east(x) | shift_west(x)
         | shift_ne(x) | shift_nw(x) | shift_se(x) | shift_sw(x);
}

inline Bitboard pawn_attacks(Color c, int s) {
    Bitboard x = bb(s);
    return c == WHITE ? (shift_nw(x) | shift_ne(x))
                      : (shift_sw(x) | shift_se(x));
}

inline Bitboard ray_attacks(int s, Bitboard occ, int df, int dr) {
    int f = file_of(s), r = rank_of(s);
    Bitboard out = 0;
    for (;;) {
        f += df; r += dr;
        if (f < 0 || f > 7 || r < 0 || r > 7) break;
        int t = r * 8 + f;
        out |= bb(t);
        if (occ & bb(t)) break;
    }
    return out;
}

inline Bitboard bishop_attacks(int s, Bitboard occ) {
    return ray_attacks(s, occ, 1, 1) | ray_attacks(s, occ, -1, 1)
         | ray_attacks(s, occ, 1, -1) | ray_attacks(s, occ, -1, -1);
}

inline Bitboard rook_attacks(int s, Bitboard occ) {
    return ray_attacks(s, occ, 1, 0) | ray_attacks(s, occ, -1, 0)
         | ray_attacks(s, occ, 0, 1) | ray_attacks(s, occ, 0, -1);
}

inline Bitboard queen_attacks(int s, Bitboard occ) {
    return bishop_attacks(s, occ) | rook_attacks(s, occ);
}

inline Bitboard pseudo_attacks(PieceType pt, int s, Color pawn_color = WHITE) {
    switch (pt) {
    case PAWN:   return pawn_attacks(pawn_color, s);
    case KNIGHT: return knight_attacks(s);
    case BISHOP: return bishop_attacks(s, 0);
    case ROOK:   return rook_attacks(s, 0);
    case QUEEN:  return queen_attacks(s, 0);
    case KING:   return king_attacks(s);
    default:     return 0;
    }
}

inline Bitboard real_attacks(PieceType pt, int s, Bitboard occ) {
    switch (pt) {
    case KNIGHT: return knight_attacks(s);
    case BISHOP: return bishop_attacks(s, occ);
    case ROOK:   return rook_attacks(s, occ);
    case QUEEN:  return queen_attacks(s, occ);
    case KING:   return king_attacks(s);
    default:     return 0;
    }
}

// -----------------------------------------------------------------------------
// FEN position
// -----------------------------------------------------------------------------

struct Position {
    std::array<Piece, 64> board{};
    Color side_to_move = WHITE;
    std::string castling;
    int ep_square = -1;
    int halfmove_clock = 0;
    int fullmove_number = 1;

    Bitboard by_color[2]{0, 0};
    Bitboard by_type[7]{0, 0, 0, 0, 0, 0, 0};

    void clear() {
        board.fill(NO_PIECE);
        side_to_move = WHITE;
        castling.clear();
        ep_square = -1;
        halfmove_clock = 0;
        fullmove_number = 1;
        by_color[0] = by_color[1] = 0;
        std::fill(std::begin(by_type), std::end(by_type), 0ULL);
    }

    void put_piece(int sq, Piece p) {
        if (p == NO_PIECE) return;
        board[sq] = p;
        by_color[piece_color(p)] |= bb(sq);
        by_type[piece_type(p)] |= bb(sq);
    }

    int king_square(Color c) const {
        Bitboard b = by_color[c] & by_type[KING];
        if (!b) throw std::runtime_error("FEN has no king for one side");
        return __builtin_ctzll(b);
    }

    Bitboard occupied() const { return by_color[0] | by_color[1]; }

    int piece_count() const { return popcount(occupied()); }

    int count(PieceType pt) const { return popcount(by_type[pt]); }
    int count_all_material() const {
        return count(PAWN) + 3 * count(KNIGHT) + 3 * count(BISHOP)
             + 5 * count(ROOK) + 9 * count(QUEEN);
    }

    static Piece char_piece(char c) {
        switch (c) {
        case 'P': return W_PAWN; case 'N': return W_KNIGHT; case 'B': return W_BISHOP;
        case 'R': return W_ROOK; case 'Q': return W_QUEEN; case 'K': return W_KING;
        case 'p': return B_PAWN; case 'n': return B_KNIGHT; case 'b': return B_BISHOP;
        case 'r': return B_ROOK; case 'q': return B_QUEEN; case 'k': return B_KING;
        default:  return NO_PIECE;
        }
    }

    void set_fen(const std::string& fen) {
        clear();

        std::array<std::string, 6> tok{};
        std::size_t pos = 0;
        for (int i = 0; i < 6; ++i) {
            std::size_t p = fen.find(' ', pos);
            if (p == std::string::npos) {
                if (i != 5) throw std::invalid_argument("FEN must have 6 fields");
                tok[i] = fen.substr(pos);
                pos = fen.size();
            } else {
                tok[i] = fen.substr(pos, p - pos);
                pos = p + 1;
            }
        }

        int sq = 56; // a8
        int rank = 7;
        int file = 0;
        for (char c : tok[0]) {
            if (c == '/') {
                if (file != 8 || rank == 0)
                    throw std::invalid_argument("bad FEN board rank data");
                --rank;
                file = 0;
                sq = rank * 8;
                continue;
            }
            if (c >= '1' && c <= '8') {
                file += c - '0';
                if (file > 8) throw std::invalid_argument("bad FEN board rank data");
                sq += c - '0';
                continue;
            }
            Piece p = char_piece(c);
            if (p == NO_PIECE) throw std::invalid_argument("bad FEN piece");
            if (file >= 8 || sq < 0 || sq >= 64)
                throw std::invalid_argument("bad FEN board");
            put_piece(sq++, p);
            ++file;
        }
        if (rank != 0 || file != 8)
            throw std::invalid_argument("bad FEN board rank data");

        side_to_move = tok[1] == "b" ? BLACK :
                       tok[1] == "w" ? WHITE :
                       throw std::invalid_argument("bad side to move");

        castling = tok[2] == "-" ? "" : tok[2];

        ep_square = -1;
        if (tok[3] != "-") {
            if (tok[3].size() != 2 ||
                tok[3][0] < 'a' || tok[3][0] > 'h' ||
                tok[3][1] < '1' || tok[3][1] > '8')
                throw std::invalid_argument("bad en-passant square");
            ep_square = (tok[3][1] - '1') * 8 + (tok[3][0] - 'a');
        }

        try {
            halfmove_clock = std::stoi(tok[4]);
            fullmove_number = std::stoi(tok[5]);
        } catch (...) {
            throw std::invalid_argument("bad FEN counters");
        }

        // NNUE itself only requires a king for each perspective.
        (void)king_square(WHITE);
        (void)king_square(BLACK);
    }
};

// -----------------------------------------------------------------------------
// SFNNv16 feature indices
// -----------------------------------------------------------------------------

struct FeatureIndex {
    // HalfKAv2_hm
    static constexpr int PS_NONE = 0;
    static constexpr int PS_W_PAWN   = 0 * 64;
    static constexpr int PS_B_PAWN   = 1 * 64;
    static constexpr int PS_W_KNIGHT = 2 * 64;
    static constexpr int PS_B_KNIGHT = 3 * 64;
    static constexpr int PS_W_BISHOP = 4 * 64;
    static constexpr int PS_B_BISHOP = 5 * 64;
    static constexpr int PS_W_ROOK   = 6 * 64;
    static constexpr int PS_B_ROOK   = 7 * 64;
    static constexpr int PS_W_QUEEN  = 8 * 64;
    static constexpr int PS_B_QUEEN  = 9 * 64;
    static constexpr int PS_KING     = 10 * 64;
    static constexpr int PS_NB       = 11 * 64;

    static constexpr int KingBuckets[64] = {
        28*PS_NB,29*PS_NB,30*PS_NB,31*PS_NB,31*PS_NB,30*PS_NB,29*PS_NB,28*PS_NB,
        24*PS_NB,25*PS_NB,26*PS_NB,27*PS_NB,27*PS_NB,26*PS_NB,25*PS_NB,24*PS_NB,
        20*PS_NB,21*PS_NB,22*PS_NB,23*PS_NB,23*PS_NB,22*PS_NB,21*PS_NB,20*PS_NB,
        16*PS_NB,17*PS_NB,18*PS_NB,19*PS_NB,19*PS_NB,18*PS_NB,17*PS_NB,16*PS_NB,
        12*PS_NB,13*PS_NB,14*PS_NB,15*PS_NB,15*PS_NB,14*PS_NB,13*PS_NB,12*PS_NB,
         8*PS_NB, 9*PS_NB,10*PS_NB,11*PS_NB,11*PS_NB,10*PS_NB, 9*PS_NB, 8*PS_NB,
         4*PS_NB, 5*PS_NB, 6*PS_NB, 7*PS_NB, 7*PS_NB, 6*PS_NB, 5*PS_NB, 4*PS_NB,
         0*PS_NB, 1*PS_NB, 2*PS_NB, 3*PS_NB, 3*PS_NB, 2*PS_NB, 1*PS_NB, 0*PS_NB
    };

    static constexpr int halfka_orient_tbl(int sq) {
        // HalfKAv2_hm: A-D -> H1 mirror, E-H -> A1 mirror.
        return file_of(sq) < 4 ? 7 : 0;
    }

    static constexpr int threat_orient_tbl(int sq) {
        // FullThreats: A-D -> A1, E-H -> H1.
        return file_of(sq) < 4 ? 0 : 7;
    }

    static int halfka_index(Color perspective, int sq, Piece p, int ksq) {
        const int flip = 56 * static_cast<int>(perspective);
        const int orient = halfka_orient_tbl(ksq) ^ flip;
        const int ps_color = static_cast<int>(piece_color(p) ^ perspective);
        const PieceType pt = piece_type(p);

        int piece_offset = 0;
        switch (pt) {
        case PAWN:   piece_offset = ps_color == 0 ? PS_W_PAWN   : PS_B_PAWN; break;
        case KNIGHT: piece_offset = ps_color == 0 ? PS_W_KNIGHT : PS_B_KNIGHT; break;
        case BISHOP: piece_offset = ps_color == 0 ? PS_W_BISHOP : PS_B_BISHOP; break;
        case ROOK:   piece_offset = ps_color == 0 ? PS_W_ROOK   : PS_B_ROOK; break;
        case QUEEN:  piece_offset = ps_color == 0 ? PS_W_QUEEN  : PS_B_QUEEN; break;
        case KING:   piece_offset = PS_KING; break;
        default:     return -1;
        }

        return (sq ^ orient) + piece_offset + KingBuckets[ksq ^ flip];
    }

    // FullThreats constants.
    static constexpr u32 ThreatHash = 0x2e6b9d04u;
    static constexpr u32 PairHash = 0x86f2b1ddu;
    static constexpr u32 HalfKAHash = 0x7f234cb8u;

    static constexpr int ThreatNumTargets[16] = {
        0,4,10,8,8,10,0,0, 0,4,10,8,8,10,0,0
    };

    static constexpr int ThreatMap[6][6] = {
        {-1, 0,-1, 1,-1,-1},
        { 0, 1, 2, 3, 4,-1},
        { 0, 1, 2, 3,-1,-1},
        { 0, 1, 2, 3,-1,-1},
        { 0, 1, 2, 3, 4,-1},
        {-1,-1,-1,-1,-1,-1}
    };

    // These arrays are initialized once by index builders.
    struct ThreatLUT {
        std::array<std::array<u32,64>,16> offsets{};
        std::array<int,16> cumulativePieceOffset{};
        std::array<int,16> cumulativeOffset{};
        std::array<std::array<std::array<u32,2>,16>,16> idx1{};
    };

    static ThreatLUT make_threat_lut() {
        ThreatLUT L{};

        const Piece pieces[12] = {
            W_PAWN,W_KNIGHT,W_BISHOP,W_ROOK,W_QUEEN,W_KING,
            B_PAWN,B_KNIGHT,B_BISHOP,B_ROOK,B_QUEEN,B_KING
        };

        int cumulativeOffset = 0;
        for (Piece p : pieces) {
            int pidx = static_cast<int>(p);
            int cpo = 0;
            for (int s = 0; s < 64; ++s) {
                L.offsets[pidx][s] = static_cast<u32>(cpo);
                PieceType pt = piece_type(p);
                if (pt != PAWN)
                    cpo += popcount(pseudo_attacks(pt, s, piece_color(p)));
                else if (s >= 8 && s <= 55)
                    cpo += popcount(pseudo_attacks(PAWN, s, piece_color(p)));
            }
            L.cumulativePieceOffset[pidx] = cpo;
            L.cumulativeOffset[pidx] = cumulativeOffset;
            cumulativeOffset += ThreatNumTargets[pidx] * cpo;
        }

        for (Piece a : pieces) {
            for (Piece d : pieces) {
                bool enemy = (static_cast<int>(a) ^ static_cast<int>(d)) == 8;
                PieceType at = piece_type(a), dt = piece_type(d);
                int map = ThreatMap[static_cast<int>(at) - 1][static_cast<int>(dt) - 1];
                bool semiExcluded = at == dt && (enemy || at != PAWN);

                int feature = L.cumulativeOffset[a]
                            + (static_cast<int>(piece_color(d))
                               * (ThreatNumTargets[a] / 2) + map)
                              * L.cumulativePieceOffset[a];

                bool excluded = map < 0;
                L.idx1[a][d][0] = excluded ? ThreatDimensions : static_cast<u32>(feature);
                L.idx1[a][d][1] =
                    (excluded || semiExcluded) ? ThreatDimensions : static_cast<u32>(feature);
            }
        }
        return L;
    }

    static const ThreatLUT& threat_lut() {
        static const ThreatLUT L = make_threat_lut();
        return L;
    }

    static int pseudo_target_ordinal(PieceType pt, Color pawn_color, int from, int to) {
        Bitboard attacks = pseudo_attacks(pt, from, pawn_color);
        int n = 0;
        while (attacks) {
            int s = pop_lsb(attacks);
            if (s == to) return n;
            ++n;
        }
        return 0; // unreachable for a valid threat
    }

    static int threat_index(Color perspective, Piece attacker, int from, int to,
                            Piece attacked, int ksq) {
        const auto& L = threat_lut();
        const int orientation = threat_orient_tbl(ksq) ^ (56 * static_cast<int>(perspective));
        const int from_o = static_cast<u8>(from) ^ orientation;
        const int to_o = static_cast<u8>(to) ^ orientation;
        const int swap = 8 * static_cast<int>(perspective);
        const int attacker_o = static_cast<int>(attacker) ^ swap;
        const int attacked_o = static_cast<int>(attacked) ^ swap;
        const int side = from_o < to_o ? 0 : 1;
        int idx2 = pseudo_target_ordinal(piece_type(static_cast<Piece>(attacker_o)),
                                         piece_color(static_cast<Piece>(attacker_o)),
                                         from_o, to_o);
        return static_cast<int>(L.idx1[attacker_o][attacked_o][side])
             + static_cast<int>(L.offsets[attacker_o][from_o]) + idx2;
    }

    static int pair_index(Color perspective, Color color, int from, int to,
                          Color pairedColor, int ksq) {
        const int orientation = threat_orient_tbl(ksq) ^ (56 * static_cast<int>(perspective));
        const int from_o = static_cast<u8>(from) ^ orientation;
        const int to_o = static_cast<u8>(to) ^ orientation;

        const Color color_o = static_cast<Color>(static_cast<int>(color) ^ static_cast<int>(perspective));
        const Color paired_o =
            static_cast<Color>(static_cast<int>(pairedColor) ^ static_cast<int>(perspective));

        const int idA = 48 * static_cast<int>(color_o) + from_o - 8;
        const int idB = 48 * static_cast<int>(paired_o) + to_o - 8;
        const int hi = std::max(idA, idB);
        const int lo = std::min(idA, idB);
        return hi * (hi - 1) / 2 + lo + 59808;
    }

    static void active_halfka(const Position& pos, Color perspective,
                              std::vector<int>& out) {
        const int ksq = pos.king_square(perspective);
        Bitboard pieces = pos.occupied();
        while (pieces) {
            int s = pop_lsb(pieces);
            out.push_back(halfka_index(perspective, s, pos.board[s], ksq));
        }
    }

    static void active_threats(const Position& pos, Color perspective,
                               std::vector<int>& out) {
        const int ksq = pos.king_square(perspective);
        const Bitboard occ = pos.occupied();
        const Bitboard pawnTargets = pos.by_type[KNIGHT] | pos.by_type[ROOK];
        const Bitboard minorTargets =
            pos.by_type[PAWN] | pos.by_type[KNIGHT] | pos.by_type[BISHOP] | pos.by_type[ROOK];
        const Bitboard queenTargets = minorTargets | pos.by_type[QUEEN];

        auto process_pawn = [&](Color c, int df, int dr) {
            Bitboard pawns = pos.by_color[c] & pos.by_type[PAWN];
            while (pawns) {
                int from = pop_lsb(pawns);
                int f = file_of(from) + df;
                int r = rank_of(from) + dr;
                if (f < 0 || f > 7 || r < 0 || r > 7) continue;
                int to = r * 8 + f;
                if (!(pawnTargets & bb(to))) continue;
                Piece attacked = pos.board[to];
                int idx = threat_index(perspective, make_piece(c, PAWN),
                                        from, to, attacked, ksq);
                if (idx < ThreatDimensions) out.push_back(idx);
            }
        };

        process_pawn(WHITE, -1, +1);
        process_pawn(WHITE, +1, +1);
        process_pawn(BLACK, -1, -1);
        process_pawn(BLACK, +1, -1);

        for (Color c : {WHITE, BLACK}) {
            for (PieceType pt : {KNIGHT, BISHOP, ROOK, QUEEN}) {
                Piece attacker = make_piece(c, pt);
                Bitboard bbp = pos.by_color[c] & pos.by_type[pt];
                Bitboard targets = (pt == KNIGHT || pt == QUEEN) ? queenTargets : minorTargets;

                while (bbp) {
                    int from = pop_lsb(bbp);
                    Bitboard attacks = real_attacks(pt, from, occ) & targets;
                    while (attacks) {
                        int to = pop_lsb(attacks);
                        Piece attacked = pos.board[to];
                        int idx = threat_index(perspective, attacker, from, to, attacked, ksq);
                        if (idx < ThreatDimensions) out.push_back(idx);
                    }
                }
            }
        }
    }

    static void active_pairs(const Position& pos, Color perspective,
                             std::vector<int>& out) {
        const int ksq = pos.king_square(perspective);
        const Bitboard white = pos.by_color[WHITE] & pos.by_type[PAWN];
        const Bitboard black = pos.by_color[BLACK] & pos.by_type[PAWN];

        Bitboard w = white;
        while (w) {
            int from = pop_lsb(w);
            Bitboard band = pawn_pair_bb(from);

            Bitboard ww = band & w;
            while (ww) {
                int to = pop_lsb(ww);
                out.push_back(pair_index(perspective, WHITE, from, to, WHITE, ksq));
            }

            Bitboard wb = band & black;
            while (wb) {
                int to = pop_lsb(wb);
                out.push_back(pair_index(perspective, WHITE, from, to, BLACK, ksq));
            }
        }

        Bitboard b = black;
        while (b) {
            int from = pop_lsb(b);
            Bitboard band = pawn_pair_bb(from);
            Bitboard bbk = band & b;
            while (bbk) {
                int to = pop_lsb(bbk);
                out.push_back(pair_index(perspective, BLACK, from, to, BLACK, ksq));
            }
        }
    }

    static constexpr u32 feature_transformer_hash() {
        u32 h = 0;
        h = rotl1(h) ^ ThreatHash;
        h = rotl1(h) ^ PairHash;
        h = rotl1(h) ^ HalfKAHash;
        return h ^ (L1 * 2);
    }
};

// -----------------------------------------------------------------------------
// Network file reader
// -----------------------------------------------------------------------------

class Reader {
public:
    explicit Reader(std::istream& in) : in_(in) {}

    u32 le_u32() {
        u8 b[4]{};
        read_bytes(b, 4);
        return u32(b[0]) | (u32(b[1]) << 8) | (u32(b[2]) << 16) | (u32(b[3]) << 24);
    }

    template<typename T>
    T le() {
        static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4, "unsupported size");
        u8 b[sizeof(T)]{};
        read_bytes(b, sizeof(T));
        u64 v = 0;
        for (std::size_t i = 0; i < sizeof(T); ++i) v |= u64(b[i]) << (8 * i);
        T out{};
        std::memcpy(&out, &v, sizeof(T));
        return out;
    }

    template<typename T>
    void raw(T* out, std::size_t count) {
        for (std::size_t i = 0; i < count; ++i) out[i] = le<T>();
    }

    template<typename T>
    void leb(T* out, std::size_t count) {
        static_assert(std::is_signed<T>::value, "LEB reader only supports signed types");
        char magic[17]{};
        read_bytes(reinterpret_cast<u8*>(magic), 17);
        static constexpr const char* M = "COMPRESSED_LEB128";
        if (std::memcmp(magic, M, 17) != 0)
            throw std::runtime_error("NNUE: missing COMPRESSED_LEB128 block");

        u32 bytes_left = le_u32();
        std::vector<u8> buf(bytes_left);
        if (bytes_left) read_bytes(buf.data(), bytes_left);

        std::size_t p = 0;
        for (std::size_t i = 0; i < count; ++i) {
            u32 result = 0;
            unsigned shift = 0;
            while (true) {
                if (p >= buf.size())
                    throw std::runtime_error("NNUE: truncated LEB128 block");
                u8 byte = buf[p++];
                result |= u32(byte & 0x7f) << (shift % 32);
                shift += 7;
                if (!(byte & 0x80)) {
                    u32 signedBits =
                        (shift >= 32 || !(byte & 0x40))
                        ? result
                        : result | ~((u32(1) << shift) - 1);
                    T x{};
                    std::memcpy(&x, &signedBits, sizeof(T));
                    out[i] = x;
                    break;
                }
                if (shift > 35)
                    throw std::runtime_error("NNUE: invalid LEB128 integer");
            }
        }

        if (p != buf.size())
            throw std::runtime_error("NNUE: malformed LEB128 byte count");
    }

private:
    std::istream& in_;

    void read_bytes(u8* dst, std::size_t n) {
        in_.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(n));
        if (in_.gcount() != static_cast<std::streamsize>(n))
            throw std::runtime_error("NNUE: unexpected end of file");
    }
};

// -----------------------------------------------------------------------------
// Feature-transformer storage
// -----------------------------------------------------------------------------

class Network {
public:
    // Disk order:
    // biases LEB
    // threat weights raw i8
    // threat PSQT LEB i32
    // pair weights raw i8
    // pair PSQT LEB i32
    // halfKA weights LEB i16
    // halfKA PSQT LEB i32
    std::array<i16, L1> biases{};
    std::vector<i16> halfka_weights;       // [HalfKADimensions][L1]
    std::vector<i32> halfka_psqt;         // [HalfKADimensions][8]
    std::vector<i8> threat_pair_weights;  // [64368][L1]
    std::vector<i32> threat_pair_psqt;    // [64368][8]

    struct Bucket {
        std::array<i32, L2> fc0_bias{};
        std::vector<i8> fc0_w; // [L1 padded][L2], stored output-major after load
        std::array<i32, L3> fc1_bias{};
        std::vector<i8> fc1_w; // [64][32]
        i32 fc2_bias = 0;
        std::vector<i8> fc2_w; // [128]
    };

    std::array<Bucket, PSQTBuckets> buckets;

    bool loaded = false;
    std::string description;
    u32 file_hash = 0;

    Network()
        : halfka_weights(static_cast<std::size_t>(HalfKADimensions) * L1),
          halfka_psqt(static_cast<std::size_t>(HalfKADimensions) * PSQTBuckets),
          threat_pair_weights(static_cast<std::size_t>(ThreatAndPairDimensions) * L1),
          threat_pair_psqt(static_cast<std::size_t>(ThreatAndPairDimensions) * PSQTBuckets) {
        for (auto& b : buckets) {
            b.fc0_w.resize(static_cast<std::size_t>(L2) * L1);
            b.fc1_w.resize(static_cast<std::size_t>(64) * L3);
            b.fc2_w.resize(128);
        }
    }

    void load(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        if (!f) throw std::runtime_error("Cannot open NNUE: " + path);
        load(f);
    }

    void load(std::istream& in) {
        Reader r(in);

        // SF19 network file header: version, whole-network hash, description size, description.
        const u32 version = r.le_u32();
        file_hash = r.le_u32();
        const u32 desc_size = r.le_u32();

        if (version != FileVersion)
            throw std::runtime_error("NNUE: unsupported file version 0x" + hex_u32(version));
        if (file_hash != ArchitectureHash) {
            throw std::runtime_error(
                "NNUE: network hash mismatch: file=0x" + hex_u32(file_hash) +
                " expected=0x" + hex_u32(ArchitectureHash));
        }

        description.resize(desc_size);
        if (desc_size) {
            in.read(description.data(), static_cast<std::streamsize>(desc_size));
            if (in.gcount() != static_cast<std::streamsize>(desc_size))
                throw std::runtime_error("NNUE: truncated description");
        }

        // Every top-level structure is preceded by its 32-bit architecture hash.
        const u32 feature_hash = r.le_u32();
        if (feature_hash != FeatureTransformerHash)
            throw std::runtime_error(
                "NNUE: feature-transformer hash mismatch: file=0x" + hex_u32(feature_hash) +
                " expected=0x" + hex_u32(FeatureTransformerHash));

        // FeatureTransformer::read_parameters() -- exact SF19 disk order.
        r.leb(biases.data(), biases.size());
        r.raw(threat_pair_weights.data(), static_cast<std::size_t>(ThreatDimensions) * L1);
        r.leb(threat_pair_psqt.data(), static_cast<std::size_t>(ThreatDimensions) * PSQTBuckets);
        r.raw(threat_pair_weights.data() + static_cast<std::size_t>(ThreatDimensions) * L1,
              static_cast<std::size_t>(PairDimensions) * L1);
        r.leb(threat_pair_psqt.data() + static_cast<std::size_t>(ThreatDimensions) * PSQTBuckets,
              static_cast<std::size_t>(PairDimensions) * PSQTBuckets);
        r.leb(halfka_weights.data(), halfka_weights.size());
        r.leb(halfka_psqt.data(), halfka_psqt.size());

        // Scalar implementation intentionally leaves the feature-transformer arrays in
        // their unpermuted disk order. SF19 only permutes for vectorized pack layouts;
        // the scalar access pattern is the identity order.

        for (int bucket = 0; bucket < PSQTBuckets; ++bucket) {
            const u32 arch_hash = r.le_u32();
            if (arch_hash != NetworkArchitectureHash)
                throw std::runtime_error(
                    "NNUE: layer-stack hash mismatch at bucket " + std::to_string(bucket) +
                    ": file=0x" + hex_u32(arch_hash) +
                    " expected=0x" + hex_u32(NetworkArchitectureHash));

            auto& b = buckets[bucket];

            // AffineTransformSparseInput<1024,32>: i32 biases + i8 weights.
            r.raw(b.fc0_bias.data(), L2);
            r.raw(b.fc0_w.data(), b.fc0_w.size());

            // Clipped/ReLU layers have no parameters.

            // AffineTransform<64,32>: i32 biases + i8 weights.
            r.raw(b.fc1_bias.data(), L3);
            r.raw(b.fc1_w.data(), b.fc1_w.size());

            // AffineTransform<128,1>.
            b.fc2_bias = r.le<i32>();
            r.raw(b.fc2_w.data(), b.fc2_w.size());
        }

        if (!in || in.peek() != std::ios::traits_type::eof())
            throw std::runtime_error("NNUE: trailing or unread network data");

        loaded = true;
    }

    std::size_t memory_bytes() const {
        std::size_t n = sizeof(*this);
        n += halfka_weights.size() * sizeof(i8);
        n += halfka_psqt.size() * sizeof(i32);
        n += threat_pair_weights.size() * sizeof(i8);
        n += threat_pair_psqt.size() * sizeof(i32);
        for (const auto& b : buckets)
            n += b.fc0_w.size() * sizeof(i8)
              + b.fc1_w.size() * sizeof(i8)
              + b.fc2_w.size() * sizeof(i8);
        return n;
    }

private:
    static std::string hex_u32(u32 x) {
        const char* d = "0123456789abcdef";
        std::string s(8, '0');
        for (int i = 7; i >= 0; --i) { s[i] = d[x & 15]; x >>= 4; }
        return s;
    }
};

// -----------------------------------------------------------------------------
// Accumulator
// -----------------------------------------------------------------------------

struct alignas(64) Accumulator {
    std::array<i16, L1> acc[2]{};
    std::array<i32, PSQTBuckets> psqt[2]{};
    bool computed[2]{false, false};

    void clear() {
        for (int p = 0; p < 2; ++p) {
            acc[p].fill(0);
            psqt[p].fill(0);
            computed[p] = false;
        }
    }
};

class AccumulatorStack {
public:
    explicit AccumulatorStack(const Network& net) : net_(net) {}

    const Accumulator& refresh(const Position& pos) {
        // This is a full refresh, independent of Stockfish's Position/StateInfo.
        // It intentionally implements the accumulator math directly.
        current_.clear();

        for (int p = 0; p < 2; ++p) {
            Color perspective = static_cast<Color>(p);

            for (int j = 0; j < L1; ++j)
                current_.acc[p][j] = net_.biases[j];

            std::vector<int> halfka, threats, pairs;
            halfka.reserve(32);
            threats.reserve(256);
            pairs.reserve(256);

            FeatureIndex::active_halfka(pos, perspective, halfka);
            FeatureIndex::active_threats(pos, perspective, threats);
            FeatureIndex::active_pairs(pos, perspective, pairs);

            // HalfKA features.
            for (int fi : halfka) {
                const i16* w = &net_.halfka_weights[static_cast<std::size_t>(fi) * L1];
                for (int j = 0; j < L1; ++j)
                    current_.acc[p][j] = sat_i16(static_cast<i32>(current_.acc[p][j]) + w[j]);

                const i32* pw = &net_.halfka_psqt[static_cast<std::size_t>(fi) * PSQTBuckets];
                for (int b = 0; b < PSQTBuckets; ++b)
                    current_.psqt[p][b] += pw[b];
            }

            // Threat and pawn-pair features share this transformer's weight array.
            for (int fi : threats) {
                const i8* w = &net_.threat_pair_weights[static_cast<std::size_t>(fi) * L1];
                for (int j = 0; j < L1; ++j)
                    current_.acc[p][j] =
                        sat_i16(static_cast<i32>(current_.acc[p][j]) + static_cast<i32>(w[j]));

                const i32* pw =
                    &net_.threat_pair_psqt[static_cast<std::size_t>(fi) * PSQTBuckets];
                for (int b = 0; b < PSQTBuckets; ++b)
                    current_.psqt[p][b] += pw[b];
            }

            for (int fi : pairs) {
                const i8* w = &net_.threat_pair_weights[static_cast<std::size_t>(fi) * L1];
                for (int j = 0; j < L1; ++j)
                    current_.acc[p][j] =
                        sat_i16(static_cast<i32>(current_.acc[p][j]) + static_cast<i32>(w[j]));

                const i32* pw =
                    &net_.threat_pair_psqt[static_cast<std::size_t>(fi) * PSQTBuckets];
                for (int b = 0; b < PSQTBuckets; ++b)
                    current_.psqt[p][b] += pw[b];
            }

            current_.computed[p] = true;
        }

        return current_;
    }

    // Minimal explicit incremental primitives. They are useful if the caller
    // already has feature indices and wants to update the exact accumulator,
    // without coupling this header to a chess move representation.
    void add_halfka(Color perspective, int feature, int sign = +1) {
        if (feature < 0 || feature >= HalfKADimensions) throw std::out_of_range("halfka feature");
        auto& a = current_.acc[perspective];
        const i16* w = &net_.halfka_weights[static_cast<std::size_t>(feature) * L1];
        for (int j = 0; j < L1; ++j)
            a[j] = sat_i16(static_cast<i32>(a[j]) + sign * static_cast<i32>(w[j]));
        const i32* pw = &net_.halfka_psqt[static_cast<std::size_t>(feature) * PSQTBuckets];
        for (int b = 0; b < PSQTBuckets; ++b)
            current_.psqt[perspective][b] += sign * pw[b];
    }

    void add_threat_or_pair(Color perspective, int feature, int sign = +1) {
        if (feature < 0 || feature >= ThreatAndPairDimensions)
            throw std::out_of_range("threat/pair feature");
        auto& a = current_.acc[perspective];
        const i8* w = &net_.threat_pair_weights[static_cast<std::size_t>(feature) * L1];
        for (int j = 0; j < L1; ++j)
            a[j] = sat_i16(static_cast<i32>(a[j]) + sign * static_cast<i32>(w[j]));
        const i32* pw = &net_.threat_pair_psqt[static_cast<std::size_t>(feature) * PSQTBuckets];
        for (int b = 0; b < PSQTBuckets; ++b)
            current_.psqt[perspective][b] += sign * pw[b];
    }

    const Accumulator& current() const noexcept { return current_; }

private:
    const Network& net_;
    Accumulator current_{};

    static i16 sat_i16(i32 x) {
        return static_cast<i16>(std::clamp(x,
            static_cast<i32>(std::numeric_limits<i16>::min()),
            static_cast<i32>(std::numeric_limits<i16>::max())));
    }
};

// -----------------------------------------------------------------------------
// Forward propagation
// -----------------------------------------------------------------------------

inline u8 clipped_relu(i32 x, int scale_bits) {
    // SF19 scalar path: clamp(input >> scale, 0, 127)
    const i32 q = arshift(x, scale_bits);
    return static_cast<u8>(std::clamp(q, 0, 127));
}

inline u8 sqr_clipped_relu(i32 x, int scale_bits) {
    // SF19 scalar path: min(127, (x*x) >> (2*scale + 7)).
    // Use i64 so a large i32 cannot overflow here.
    const i64 xx = static_cast<i64>(x) * static_cast<i64>(x);
    const i64 q = xx >> (2 * scale_bits + 7);
    return static_cast<u8>(std::min<i64>(127, q));
}

inline i32 affine32x32(const Network::Bucket& b, const u8* input) {
    std::array<i32, L2> out{};
    for (int o = 0; o < L2; ++o) {
        i64 s = b.fc0_bias[o];
        const i8* w = &b.fc0_w[static_cast<std::size_t>(o) * L1];
        for (int i = 0; i < L1; ++i)
            s += static_cast<i64>(static_cast<u8>(input[i])) * static_cast<i64>(w[i]);
        out[o] = static_cast<i32>(s);
    }
    // This function returns in a static thread-local-ish scratch below.
    return out[0];
}

// -----------------------------------------------------------------------------
// Evaluator result
// -----------------------------------------------------------------------------

enum class ScoreMode {
    raw_stm,    // combined NNUE internal units, side-to-move POV
    raw_white,  // combined NNUE internal units, White POV
    cp_stm,     // SF19 to_cp, side-to-move POV
    cp_white    // SF19 to_cp, White POV
};

struct Result {
    Color side_to_move = WHITE;
    int bucket = 0;

    i32 psqt_internal = 0;
    i32 positional_internal = 0;
    i32 raw_internal = 0;

    i32 value_stm = 0;
    i32 value_white = 0;

    i32 cp_stm = 0;
    i32 cp_white = 0;

    i32 selected(ScoreMode mode) const noexcept {
        switch (mode) {
        case ScoreMode::raw_stm:   return value_stm;
        case ScoreMode::raw_white: return value_white;
        case ScoreMode::cp_stm:    return cp_stm;
        case ScoreMode::cp_white:  return cp_white;
        }
        return value_stm;
    }
};

inline i32 sf19_to_cp(i32 value, const Position& pos) {
    int material = pos.count_all_material();
    double m = static_cast<double>(std::clamp(material, 17, 78)) / 58.0;

    constexpr double as[] = {
        -142.72052667, 372.35176398, -340.71073572, 415.23490212
    };
    const double a = (((as[0] * m + as[1]) * m + as[2]) * m) + as[3];
    return static_cast<i32>(std::llround(100.0 * static_cast<double>(value) / a));
}

class Evaluator {
public:
    Evaluator() = default;
    explicit Evaluator(const std::string& nnue_path) { load(nnue_path); }

    void load(const std::string& nnue_path) {
        net_.load(nnue_path);
    }

    bool loaded() const noexcept { return net_.loaded; }
    const Network& network() const noexcept { return net_; }
    Network& network() noexcept { return net_; }

    Result evaluate(const Position& pos) {
        if (!net_.loaded) throw std::runtime_error("NNUE network not loaded");

        AccumulatorStack stack(net_);
        const Accumulator& a = stack.refresh(pos);

        const int bucket = std::clamp((pos.piece_count() - 1) / 4, 0, 7);

        // Feature transformer output:
        // for each perspective, 512 pairs are clipped and multiplied, /512.
        // It creates a 1024-byte vector in perspective order:
        // [perspective 0 512 values][perspective 1 512 values].
        std::array<u8, L1> transformed{};

        const Color stm = pos.side_to_move;
        const Color opp = static_cast<Color>(stm ^ 1);

        const int psqt = (a.psqt[stm][bucket] - a.psqt[opp][bucket]) / 2;

        transform_side(a.acc[stm], transformed.data());
        transform_side(a.acc[opp], transformed.data() + 512);

        const auto& B = net_.buckets[bucket];

        // FC0: sparse in SF19, but mathematically identical to dense scalar.
        std::array<i32, L2> fc0{};
        for (int o = 0; o < L2; ++o) {
            i64 s = B.fc0_bias[o];
            const i8* w = &B.fc0_w[static_cast<std::size_t>(o) * L1];
            for (int i = 0; i < L1; ++i)
                s += static_cast<i64>(transformed[i]) * static_cast<i64>(w[i]);
            fc0[o] = static_cast<i32>(s);
        }

        std::array<u8, L2> sqr0{}, clip0{};
        for (int i = 0; i < L2; ++i) {
            sqr0[i] = sqr_clipped_relu(fc0[i], WeightScaleBits + 1); // 7
            clip0[i] = clipped_relu(fc0[i], WeightScaleBits + 1);    // 7
        }

        // Concatenated activations are [sqr0(32), clip0(32)].
        std::array<u8, 64> cat1{};
        for (int i = 0; i < 32; ++i) {
            cat1[i] = sqr0[i];
            cat1[32 + i] = clip0[i];
        }

        std::array<i32, L3> fc1{};
        for (int o = 0; o < L3; ++o) {
            i64 s = B.fc1_bias[o];
            const i8* w = &B.fc1_w[static_cast<std::size_t>(o) * 64];
            for (int i = 0; i < 64; ++i)
                s += static_cast<i64>(cat1[i]) * static_cast<i64>(w[i]);
            fc1[o] = static_cast<i32>(s);
        }

        std::array<u8, L3> sqr1{}, clip1{};
        for (int i = 0; i < L3; ++i) {
            sqr1[i] = sqr_clipped_relu(fc1[i], WeightScaleBits); // 6
            clip1[i] = clipped_relu(fc1[i], WeightScaleBits);    // 6
        }

        // FC2 input = [sqr0, clip0, sqr1, clip1] = 96 bytes.
        std::array<u8, 128> cat2{};
        for (int i = 0; i < 32; ++i) {
            cat2[i] = sqr0[i];
            cat2[32 + i] = clip0[i];
            cat2[64 + i] = sqr1[i];
            cat2[96 + i] = clip1[i];
        }

        i64 fwd = B.fc2_bias;
        for (int i = 0; i < 128; ++i)
            fwd += static_cast<i64>(cat2[i]) * static_cast<i64>(B.fc2_w[i]);

        // SF19's learned skip connection.
        fwd += static_cast<i64>(fc0[30]) - static_cast<i64>(fc0[31]);

        // NetworkArchitecture::propagate()
        const i64 multiplier = 600LL * OutputScale;
        const i64 denominator = static_cast<i64>(HiddenOneVal)
                              * static_cast<i64>(1ULL << WeightScaleBits) * 2LL;
        const i32 positional = static_cast<i32>((fwd * multiplier) / denominator);

        // Network::evaluate() divides each branch by OutputScale separately.
        const i32 value_stm = psqt / OutputScale + positional / OutputScale;
        const i32 value_white = (stm == WHITE) ? value_stm : -value_stm;

        Result r;
        r.side_to_move = stm;
        r.bucket = bucket;
        r.psqt_internal = psqt;
        r.positional_internal = positional;
        r.raw_internal = value_stm;
        r.value_stm = value_stm;
        r.value_white = value_white;
        r.cp_stm = sf19_to_cp(value_stm, pos);
        r.cp_white = sf19_to_cp(value_white, pos);
        return r;
    }

    Result evaluate_fen(const std::string& fen) {
        Position pos;
        pos.set_fen(fen);
        return evaluate(pos);
    }

private:
    Network net_{};

    static void transform_side(const std::array<i16, L1>& acc, u8* out) {
        for (int j = 0; j < 512; ++j) {
            const i32 a = std::clamp<i32>(acc[j], 0, FtMaxVal);
            const i32 b = std::clamp<i32>(acc[j + 512], 0, FtMaxVal);
            out[j] = static_cast<u8>((a * b) >> 9);
        }
    }
};

// -----------------------------------------------------------------------------
// Convenience one-shot API
// -----------------------------------------------------------------------------

inline Result probe(const std::string& nnue_path, const std::string& fen,
                    ScoreMode mode = ScoreMode::cp_stm) {
    Evaluator e(nnue_path);
    Position p;
    p.set_fen(fen);
    Result r = e.evaluate(p);
    (void)mode;
    return r;
}

} // namespace sfnnv16
