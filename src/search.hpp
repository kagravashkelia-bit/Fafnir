// =============================================================================
//  search.hpp  --  fafnir search + MovePicker (arquitectura de Stockfish dev)
//
//  Contenido:
//    * TT de Stockfish (clusters de 3 entradas, generacion, reemplazo por edad)
//    * Historiales: main, low-ply, captura, continuation (x2x2), pawn,
//      correction (pawn / minor / non-pawn / continuation), ttMoveHistory
//    * MovePicker propio con las etapas de SF (TT, buenas capturas, quiets,
//      malas capturas, malos quiets, evasiones, probcut, qsearch)
//    * SEE con pins, cuckoo (repeticion inminente), draw por repeticion/50 jugadas
//    * search<Root/PV/NonPV> + qsearch<PV/NonPV>: razoring, futility, NMP con
//      verificacion, IIR, ProbCut, extension singular + multicut + negativas,
//      LMR, pruning superficial, correction history, etc.
//    * Gestion de tiempo (soft/hard, ponder)
//
//  Interfaz publica (usada por SearchController.cpp / uci.cpp):
//    MAX_PLY, ThreadWorker, SearchAbortedException, negamax(), init_search_time(),
//    wait_for_ponderhit(), search_start_time(_snapshot), search_uses_ponder,
//    ponderhit_requested, search_aggressivity, search_time_mutex,
//    apply_aggressivity_to_optimism(), format_score_uci(), resize_tt(), clear_tt()
// =============================================================================
#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <thread>
#include <vector>

#include "chesslib.hpp"
#include "evaluate.hpp"
#include "SearchController.hpp"

#ifdef sync_cout
#undef sync_cout
#endif
#ifdef MAX_PLY
#undef MAX_PLY
#endif

#if defined(__GNUC__) || defined(__clang__)
#define FB_INLINE inline __attribute__((always_inline))
#define FB_LIKELY(x) __builtin_expect(!!(x), 1)
#define FB_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define FB_PREFETCH(p) __builtin_prefetch(p)
#else
#define FB_INLINE inline
#define FB_LIKELY(x) (x)
#define FB_UNLIKELY(x) (x)
#define FB_PREFETCH(p) ((void)0)
#endif

// Poner a 0 para desactivar null-move pruning (ver notas: la sonda NNUE no
// tiene push de null move, asi que cada null move fuerza un resync).
#ifndef FB_NULL_MOVE_PRUNING
#define FB_NULL_MOVE_PRUNING 1
#endif

// ============================================================================
//  API PUBLICA (global)
// ============================================================================

inline constexpr int MAX_PLY = 246;

class SearchAbortedException : public std::exception {
public:
    const char* what() const noexcept override { return "search aborted"; }
};

inline int search_aggressivity = 0;               // UCI "Aggressivity" 0..100
inline std::mutex search_time_mutex;
inline std::atomic<bool> ponderhit_requested{false};
inline std::atomic<bool> search_uses_ponder{false};
inline std::chrono::high_resolution_clock::time_point search_start_time{};

namespace fb {

using Value = int;
using Depth = int;
using Key   = std::uint64_t;
using Move  = chess::Move;

// ---------------------------------------------------------------------------
//  Valores
// ---------------------------------------------------------------------------
constexpr Value VALUE_ZERO     = 0;
constexpr Value VALUE_DRAW     = 0;
constexpr Value VALUE_MATE     = 32000;
constexpr Value VALUE_INFINITE = 32001;
constexpr Value VALUE_NONE     = 32002;

constexpr Value VALUE_MATE_IN_MAX_PLY  = VALUE_MATE - MAX_PLY;
constexpr Value VALUE_MATED_IN_MAX_PLY = -VALUE_MATE_IN_MAX_PLY;
constexpr Value VALUE_TB                 = VALUE_MATE_IN_MAX_PLY - 1;
constexpr Value VALUE_TB_WIN_IN_MAX_PLY  = VALUE_TB - MAX_PLY;
constexpr Value VALUE_TB_LOSS_IN_MAX_PLY = -VALUE_TB_WIN_IN_MAX_PLY;

constexpr Depth DEPTH_QS           = 0;
constexpr Depth DEPTH_UNSEARCHED   = -2;
constexpr Depth DEPTH_ENTRY_OFFSET = -3;

// Valores de pieza de SF (indexados por chess::PieceType: P N B R Q K NONE)
constexpr int PIECE_VALUE[7] = {208, 781, 825, 1276, 2538, 0, 0};
constexpr int PT_NONE = 6;

// Normalizacion para mostrar cp en UCI (SF 17.x). Ajustable.
constexpr int NORMALIZE_TO_PAWN_VALUE = 356;

FB_INLINE bool is_valid(Value v) { return v != VALUE_NONE; }
FB_INLINE bool is_win(Value v) { return v >= VALUE_TB_WIN_IN_MAX_PLY; }
FB_INLINE bool is_loss(Value v) { return v <= VALUE_TB_LOSS_IN_MAX_PLY; }
FB_INLINE bool is_decisive(Value v) { return is_win(v) || is_loss(v); }
FB_INLINE Value mate_in(int ply) { return VALUE_MATE - ply; }
FB_INLINE Value mated_in(int ply) { return -VALUE_MATE + ply; }

FB_INLINE Value value_to_tt(Value v, int ply) {
    return is_win(v) ? v + ply : is_loss(v) ? v - ply : v;
}

FB_INLINE Value value_from_tt(Value v, int ply, int r50c) {
    if (!is_valid(v)) return VALUE_NONE;
    if (is_win(v)) {
        if (v >= VALUE_MATE_IN_MAX_PLY && VALUE_MATE - v > 100 - r50c) return VALUE_TB_WIN_IN_MAX_PLY - 1;
        if (VALUE_TB - v > 100 - r50c) return VALUE_TB_WIN_IN_MAX_PLY - 1;
        return v - ply;
    }
    if (is_loss(v)) {
        if (v <= VALUE_MATED_IN_MAX_PLY && VALUE_MATE + v > 100 - r50c) return VALUE_TB_LOSS_IN_MAX_PLY + 1;
        if (VALUE_TB + v > 100 - r50c) return VALUE_TB_LOSS_IN_MAX_PLY + 1;
        return v + ply;
    }
    return v;
}

// ---------------------------------------------------------------------------
//  Utilidades de jugadas / tablero
// ---------------------------------------------------------------------------
FB_INLINE int  mv_from(Move m) { return (m.move() >> 6) & 63; }
FB_INLINE int  mv_to(Move m) { return m.move() & 63; }
FB_INLINE bool mv_ok(Move m) { return mv_from(m) != mv_to(m); }      // ni none ni null
FB_INLINE int  mv_from_to(Move m) { return m.move() & 0xFFF; }
FB_INLINE bool mv_none(Move m) { return m.move() == 0; }
FB_INLINE Move make_null() { return Move(static_cast<std::uint16_t>(Move::NULL_MOVE)); }
FB_INLINE Move make_none() { return Move(static_cast<std::uint16_t>(Move::NO_MOVE)); }

FB_INLINE int pt_idx(chess::PieceType t) { return static_cast<int>(t); }
FB_INLINE int pc_idx(chess::Piece p) { return static_cast<int>(p); }       // 0..11, NONE=12
constexpr int PIECE_NONE_IDX = 12;

FB_INLINE bool capture_stage(const chess::Board& b, Move m) {
    return b.isCapture(m) ||
           (m.typeOf() == Move::PROMOTION && m.promotionType() == chess::PieceType::QUEEN);
}

FB_INLINE std::uint64_t bits(const chess::Bitboard& b) { return b.getBits(); }
FB_INLINE int lsb64(std::uint64_t b) { return __builtin_ctzll(b); }
FB_INLINE int pop_lsb64(std::uint64_t& b) { int s = __builtin_ctzll(b); b &= b - 1; return s; }
FB_INLINE int popcount64(std::uint64_t b) { return __builtin_popcountll(b); }
FB_INLINE bool more_than_one(std::uint64_t b) { return b & (b - 1); }

FB_INLINE std::uint64_t bb_knight(int s) { return bits(chess::attacks::knight(chess::Square(s))); }
FB_INLINE std::uint64_t bb_king(int s) { return bits(chess::attacks::king(chess::Square(s))); }
FB_INLINE std::uint64_t bb_bishop(int s, std::uint64_t o) {
    return bits(chess::attacks::bishop(chess::Square(s), chess::Bitboard(o)));
}
FB_INLINE std::uint64_t bb_rook(int s, std::uint64_t o) {
    return bits(chess::attacks::rook(chess::Square(s), chess::Bitboard(o)));
}
FB_INLINE std::uint64_t bb_pawn(int color, int s) {
    return bits(chess::attacks::pawn(chess::Color(color), chess::Square(s)));
}

FB_INLINE std::uint64_t pcs(const chess::Board& b, chess::PieceType::underlying t) {
    return bits(b.pieces(chess::PieceType(t)));
}
FB_INLINE std::uint64_t pcs(const chess::Board& b, chess::PieceType::underlying t, int color) {
    return bits(b.pieces(chess::PieceType(t), chess::Color(color)));
}

// ---------------------------------------------------------------------------
//  Hash de estructuras (SF usa claves incrementales; aqui se derivan de bitboards)
// ---------------------------------------------------------------------------
FB_INLINE std::uint32_t hidx(std::uint64_t k, int nbits) {
    return static_cast<std::uint32_t>((k * 0xD6E8FEB86659FD93ULL) >> (64 - nbits));
}
FB_INLINE std::uint64_t pawn_key(const chess::Board& b) {
    return pcs(b, chess::PieceType::PAWN, 0) * 0x9E3779B97F4A7C15ULL ^
           (pcs(b, chess::PieceType::PAWN, 1) * 0xC2B2AE3D27D4EB4FULL + 0x165667B19E3779F9ULL);
}
FB_INLINE std::uint64_t minor_key(const chess::Board& b) {
    using PT = chess::PieceType;
    return pcs(b, PT::KNIGHT, 0) * 0x9E3779B97F4A7C15ULL ^ pcs(b, PT::BISHOP, 0) * 0xC2B2AE3D27D4EB4FULL ^
           pcs(b, PT::KNIGHT, 1) * 0x165667B19E3779F9ULL ^ pcs(b, PT::BISHOP, 1) * 0x27D4EB2F165667C5ULL;
}
FB_INLINE std::uint64_t nonpawn_key(const chess::Board& b, int c) {
    using PT = chess::PieceType;
    return pcs(b, PT::KNIGHT, c) * 0x9E3779B97F4A7C15ULL ^ pcs(b, PT::BISHOP, c) * 0xC2B2AE3D27D4EB4FULL ^
           pcs(b, PT::ROOK, c) * 0x165667B19E3779F9ULL ^ pcs(b, PT::QUEEN, c) * 0x27D4EB2F165667C5ULL ^
           pcs(b, PT::KING, c) * 0x85EBCA77C2B2AE63ULL;
}

constexpr int PAWN_HISTORY_SIZE        = 8192;
constexpr int CORRECTION_HISTORY_SIZE  = 32768;
constexpr int CORRECTION_HISTORY_LIMIT = 1024;
constexpr int LOW_PLY_HISTORY_SIZE     = 5;

// ============================================================================
//  TRANSPOSITION TABLE (Stockfish)
// ============================================================================
enum Bound : std::uint8_t { BOUND_NONE = 0, BOUND_UPPER = 1, BOUND_LOWER = 2, BOUND_EXACT = 3 };

struct TTData {
    Move  move;
    Value value;
    Value eval;
    Depth depth;
    Bound bound;
    bool  is_pv;
};

constexpr int GENERATION_BITS  = 3;
constexpr int GENERATION_DELTA = 1 << GENERATION_BITS;
constexpr int GENERATION_CYCLE = 255 + GENERATION_DELTA;
constexpr int GENERATION_MASK  = (0xFF << GENERATION_BITS) & 0xFF;

struct TTEntry {
    std::uint16_t key16;
    std::uint8_t  depth8;
    std::uint8_t  genBound8;
    std::uint16_t move16;
    std::int16_t  value16;
    std::int16_t  eval16;

    FB_INLINE bool is_occupied() const { return depth8 != 0; }
    FB_INLINE TTData read() const {
        return TTData{Move(move16), static_cast<Value>(value16), static_cast<Value>(eval16),
                      static_cast<Depth>(depth8) + DEPTH_ENTRY_OFFSET, static_cast<Bound>(genBound8 & 3),
                      static_cast<bool>(genBound8 & 4)};
    }
    FB_INLINE int relative_age(std::uint8_t generation8) const {
        return (GENERATION_CYCLE + generation8 - genBound8) & GENERATION_MASK;
    }
    FB_INLINE void save(Key k, Value v, bool pv, Bound b, Depth d, Move m, Value ev, std::uint8_t generation8) {
        // Conservar la jugada vieja si no hay nueva
        if (m.move() || static_cast<std::uint16_t>(k) != key16) move16 = m.move();
        // Sobrescribir entradas menos valiosas
        if (b == BOUND_EXACT || static_cast<std::uint16_t>(k) != key16 ||
            d - DEPTH_ENTRY_OFFSET + 2 * pv > depth8 - 4 || relative_age(generation8)) {
            key16     = static_cast<std::uint16_t>(k);
            depth8    = static_cast<std::uint8_t>(d - DEPTH_ENTRY_OFFSET);
            genBound8 = static_cast<std::uint8_t>(generation8 | static_cast<std::uint8_t>(pv) << 2 | b);
            value16   = static_cast<std::int16_t>(v);
            eval16    = static_cast<std::int16_t>(ev);
        }
    }
};

struct TTCluster {
    TTEntry entry[3];
    char    padding[2];
};
static_assert(sizeof(TTCluster) == 32, "TT cluster must be 32 bytes");

FB_INLINE std::uint64_t mul_hi64(std::uint64_t a, std::uint64_t b) {
#if defined(__SIZEOF_INT128__)
    __extension__ typedef unsigned __int128 u128;
    return static_cast<std::uint64_t>((static_cast<u128>(a) * b) >> 64);
#else
    const std::uint64_t aL = static_cast<std::uint32_t>(a), aH = a >> 32;
    const std::uint64_t bL = static_cast<std::uint32_t>(b), bH = b >> 32;
    const std::uint64_t c1 = (aL * bL) >> 32;
    const std::uint64_t c2 = aH * bL + c1;
    const std::uint64_t c3 = aL * bH + static_cast<std::uint32_t>(c2);
    return aH * bH + (c2 >> 32) + (c3 >> 32);
#endif
}

struct TTProbe {
    bool     hit;
    TTData   data;
    TTEntry* writer;
};

class TranspositionTable {
public:
    TranspositionTable() = default;
    ~TranspositionTable() { release(); }
    TranspositionTable(const TranspositionTable&)            = delete;
    TranspositionTable& operator=(const TranspositionTable&) = delete;

    void resize(std::size_t mb) {
        const std::size_t count = std::max<std::size_t>(1, mb * 1024 * 1024 / sizeof(TTCluster));
        auto* p = static_cast<TTCluster*>(::operator new(count * sizeof(TTCluster), std::align_val_t(64), std::nothrow));
        if (!p) throw std::bad_alloc();
        release();
        table_        = p;
        clusterCount_ = count;
        generation8_  = 0;
        clear();
    }
    void clear() {
        if (table_) std::memset(static_cast<void*>(table_), 0, clusterCount_ * sizeof(TTCluster));
        generation8_ = 0;
    }
    void new_search() { generation8_ = static_cast<std::uint8_t>(generation8_ + GENERATION_DELTA); }
    std::uint8_t generation() const { return generation8_; }

    FB_INLINE TTEntry* first_entry(Key key) const {
        return &table_[mul_hi64(key, clusterCount_)].entry[0];
    }
    FB_INLINE void prefetch(Key key) const { FB_PREFETCH(first_entry(key)); }

    FB_INLINE TTProbe probe(Key key) const {
        TTEntry* const      tte   = first_entry(key);
        const std::uint16_t key16 = static_cast<std::uint16_t>(key);
        for (int i = 0; i < 3; ++i)
            if (tte[i].key16 == key16) return {tte[i].is_occupied(), tte[i].read(), &tte[i]};
        TTEntry* replace = tte;
        for (int i = 1; i < 3; ++i)
            if (replace->depth8 - replace->relative_age(generation8_) * 2 >
                tte[i].depth8 - tte[i].relative_age(generation8_) * 2)
                replace = &tte[i];
        return {false, TTData{make_none(), VALUE_NONE, VALUE_NONE, DEPTH_ENTRY_OFFSET, BOUND_NONE, false}, replace};
    }

    int hashfull() const {
        if (!table_) return 0;
        int cnt = 0;
        const std::size_t n = std::min<std::size_t>(1000, clusterCount_);
        for (std::size_t i = 0; i < n; ++i)
            for (int j = 0; j < 3; ++j)
                cnt += table_[i].entry[j].is_occupied() &&
                       (table_[i].entry[j].genBound8 & GENERATION_MASK) == generation8_;
        return static_cast<int>(cnt * 1000 / (3 * n));
    }

private:
    void release() {
        if (table_) ::operator delete(table_, std::align_val_t(64));
        table_ = nullptr;
        clusterCount_ = 0;
    }
    TTCluster*    table_        = nullptr;
    std::size_t   clusterCount_ = 0;
    std::uint8_t  generation8_  = 0;
};

// La TT es un singleton perezoso (evita el "static init order fiasco" con g_engine)
inline TranspositionTable& tt_instance() {
    static TranspositionTable t;
    return t;
}

FB_INLINE void tt_write(TTEntry* e, Key key, Value v, bool pv, Bound b, Depth d, Move m, Value ev,
                        std::uint8_t gen) {
    e->save(key, v, pv, b, d, m, ev, gen);
}

// ============================================================================
//  HISTORIALES
// ============================================================================
template<int D>
FB_INLINE void stat_update(std::int16_t& entry, int bonus) {
    const int clamped = std::clamp(bonus, -D, D);
    entry = static_cast<std::int16_t>(entry + clamped - entry * std::abs(clamped) / D);
}

struct PieceToHist {                      // [pieza 0..12][casilla]
    std::int16_t v[13 * 64];
};

struct WorkerData {
    std::int16_t mainHistory[2][4096];                 // D=7183
    std::int16_t lowPlyHistory[LOW_PLY_HISTORY_SIZE][4096];   // D=7183
    std::int16_t captureHistory[13][64][7];            // D=10692
    PieceToHist  continuationHistory[2][2][13 * 64];   // D=29952
    std::int16_t pawnHistory[PAWN_HISTORY_SIZE][13 * 64];     // D=8192
    std::int16_t pawnCorrection[CORRECTION_HISTORY_SIZE][2];
    std::int16_t minorCorrection[CORRECTION_HISTORY_SIZE][2];
    std::int16_t nonPawnCorrection[CORRECTION_HISTORY_SIZE][2][2];
    PieceToHist  continuationCorrection[13 * 64];
    std::int16_t ttMoveHistory;                        // D=8192

    void clear() {
        for (auto& a : mainHistory) for (auto& e : a) e = 68;
        for (auto& a : lowPlyHistory) for (auto& e : a) e = 97;
        for (auto& a : captureHistory) for (auto& b : a) for (auto& e : b) e = -689;
        for (auto& a : continuationHistory) for (auto& b : a) for (auto& c : b) for (auto& e : c.v) e = -529;
        for (auto& a : pawnHistory) for (auto& e : a) e = -1238;
        for (auto& a : pawnCorrection) for (auto& e : a) e = 5;
        for (auto& a : minorCorrection) for (auto& e : a) e = 0;
        for (auto& a : nonPawnCorrection) for (auto& b : a) for (auto& e : b) e = 0;
        for (auto& c : continuationCorrection) for (auto& e : c.v) e = 8;
        ttMoveHistory = 0;
    }
};

// Los historiales persisten entre "go" (como en SF) y se limpian en ucinewgame/clear_tt
inline std::mutex& worker_pool_mutex() { static std::mutex m; return m; }
inline std::vector<std::unique_ptr<WorkerData>>& worker_pool() {
    static std::vector<std::unique_ptr<WorkerData>> p;
    return p;
}
inline WorkerData* worker_data(int id) {
    std::lock_guard<std::mutex> lk(worker_pool_mutex());
    auto& pool = worker_pool();
    if (static_cast<int>(pool.size()) <= id) pool.resize(static_cast<std::size_t>(id) + 1);
    if (!pool[id]) {
        pool[id].reset(new WorkerData());
        pool[id]->clear();
    }
    return pool[id].get();
}
inline void clear_worker_data() {
    std::lock_guard<std::mutex> lk(worker_pool_mutex());
    for (auto& p : worker_pool())
        if (p) p->clear();
}

// ============================================================================
//  STACK / ROOT MOVES / CONTADOR
// ============================================================================
struct Stack {
    PieceToHist* continuationHistory;
    PieceToHist* continuationCorrectionHistory;
    Move         currentMove;
    Move         excludedMove;
    int          ply;
    int          staticEval;
    int          statScore;
    int          moveCount;
    int          cutoffCnt;
    int          reduction;
    int          pliesFromNull;
    int          capturedPt;     // tipo de pieza capturada por currentMove (PT_NONE si ninguna)
    bool         inCheck;
    bool         ttPv;
    bool         ttHit;
};

struct RootMove {
    Move         move;
    int          score            = -VALUE_INFINITE;
    int          previousScore    = -VALUE_INFINITE;
    int          averageScore     = -VALUE_INFINITE;
    int          meanSquaredScore = -VALUE_INFINITE * VALUE_INFINITE;
    std::uint64_t effort          = 0;
};

struct ExtMove {
    Move move;
    int  value;
};

}  // namespace fb

// Contador de nodos legible desde otros hilos sin data race (relaxed)
struct RelaxedCounter {
    std::atomic<std::uint64_t> v{0};
    RelaxedCounter() = default;
    void inc() { v.store(v.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed); }
    operator std::uint64_t() const { return v.load(std::memory_order_relaxed); }
    RelaxedCounter& operator=(std::uint64_t x) { v.store(x, std::memory_order_relaxed); return *this; }
};

struct ThreadWorker {
    // --- campos usados por SearchController.cpp ---
    int                        id        = 0;
    int                        sel_depth = 0;
    int                        optimism  = 0;
    RelaxedCounter             nodos_busqueda;
    std::vector<std::uint64_t> pos_history;
    int                        pv_length[MAX_PLY + 4] = {};
    chess::Move                pv_table[MAX_PLY + 4][MAX_PLY + 4] = {};

    // --- estado interno de la busqueda ---
    fb::WorkerData*           data = nullptr;
    fb::TranspositionTable*   tt   = nullptr;
    RubiEvalContext*          ctx  = nullptr;
    chess::Color              root_stm{};
    fb::Stack                 stack[MAX_PLY + 16] = {};
    fb::ExtMove               move_buf[2][MAX_PLY + 8][256];   // banco 1 = nodos con excludedMove
    std::vector<fb::RootMove> root_moves;

    int  nmpMinPly       = 0;
    int  rootDepth       = 0;
    int  completedDepth  = 0;
    int  rootDelta       = 1;
    int  callsCnt        = 0;
    bool root_initialized = false;

    // gestion de tiempo (hilo principal)
    int          cur_iter_depth     = 0;
    int          bestMoveChanges    = 0;
    double       totBestMoveChanges = 0;
    int          lastBestMoveDepth  = 0;
    std::uint16_t lastBestMove      = 0;
    double       previousTimeReduction = 1.0;
    int          iterValue[4]       = {fb::VALUE_NONE, fb::VALUE_NONE, fb::VALUE_NONE, fb::VALUE_NONE};
    int          iterIdx            = 0;
    int          bestPreviousAverageScore = fb::VALUE_INFINITE;
};

inline int apply_aggressivity_to_optimism(int raw) {
    // Formula de SF: optimism = 142 * avg / (|avg| + 91). "Aggressivity" 0 = SF puro;
    // cada punto escala el optimismo (100 -> x2).
    const int base = 142 * raw / (std::abs(raw) + 91);
    return base * (100 + std::clamp(search_aggressivity, 0, 100)) / 100;
}

inline std::string format_score_uci(int v) {
    using namespace fb;
    if (v >= VALUE_MATE_IN_MAX_PLY && v <= VALUE_MATE) return "mate " + std::to_string((VALUE_MATE - v + 1) / 2);
    if (v <= VALUE_MATED_IN_MAX_PLY && v >= -VALUE_MATE) return "mate " + std::to_string(-(VALUE_MATE + v) / 2);
    return "cp " + std::to_string(100 * v / NORMALIZE_TO_PAWN_VALUE);
}

inline void resize_tt(std::size_t mb) { fb::tt_instance().resize(mb); }
inline void clear_tt() {
    fb::tt_instance().clear();
    fb::clear_worker_data();
}

// ============================================================================
//  TABLAS ESTATICAS (between, cuckoo) -- inicializadas perezosamente
// ============================================================================
namespace fb {

struct Tables {
    std::uint64_t between[64][64];
    std::uint64_t cuckoo[8192];
    std::uint16_t cuckooMove[8192];
    std::uint64_t zside;
    int           cuckooCount;

    static FB_INLINE std::uint32_t H1(std::uint64_t h) { return static_cast<std::uint32_t>(h & 0x1fff); }
    static FB_INLINE std::uint32_t H2(std::uint64_t h) { return static_cast<std::uint32_t>((h >> 16) & 0x1fff); }

    static std::string fen_of(const char* b, bool white) {
        std::string fen;
        for (int r = 7; r >= 0; --r) {
            int empty = 0;
            for (int f = 0; f < 8; ++f) {
                const char c = b[r * 8 + f];
                if (c == '.') { ++empty; continue; }
                if (empty) { fen += static_cast<char>('0' + empty); empty = 0; }
                fen += c;
            }
            if (empty) fen += static_cast<char>('0' + empty);
            if (r) fen += '/';
        }
        fen += white ? " w - - 0 1" : " b - - 0 1";
        return fen;
    }
    static std::uint64_t hash_of(const char* b, bool white) {
        chess::Board bd(fen_of(b, white));
        return bd.hash();
    }
    static int cheb(int a, int b) { return std::max(std::abs((a >> 3) - (b >> 3)), std::abs((a & 7) - (b & 7))); }

    Tables() {
        // ---- between ----
        for (int s1 = 0; s1 < 64; ++s1)
            for (int s2 = 0; s2 < 64; ++s2) {
                std::uint64_t bt = 0;
                if (s1 != s2) {
                    const int dr = (s2 >> 3) - (s1 >> 3), df = (s2 & 7) - (s1 & 7);
                    if (dr == 0 || df == 0 || std::abs(dr) == std::abs(df)) {
                        const int sr = (dr > 0) - (dr < 0), sf = (df > 0) - (df < 0);
                        int r = (s1 >> 3) + sr, f = (s1 & 7) + sf;
                        while (r != (s2 >> 3) || f != (s2 & 7)) {
                            bt |= 1ULL << (r * 8 + f);
                            r += sr;
                            f += sf;
                        }
                    }
                }
                between[s1][s2] = bt;
            }

        // ---- cuckoo (claves derivadas de hashes del Board, ya que Zobrist es privado) ----
        char b[64];
        auto clear_b = [&] { std::memset(b, '.', 64); };
        clear_b();
        b[0] = 'K'; b[63] = 'k';
        zside = hash_of(b, true) ^ hash_of(b, false);

        static const char CH[2][6] = {{'P', 'N', 'B', 'R', 'Q', 'K'}, {'p', 'n', 'b', 'r', 'q', 'k'}};
        static std::uint64_t Z[2][6][64];
        std::memset(Z, 0, sizeof(Z));
        for (int c = 0; c < 2; ++c) {
            for (int pt = 1; pt <= 4; ++pt)
                for (int sq = 0; sq < 64; ++sq) {
                    clear_b();
                    int wk = (sq == 0) ? 7 : 0, bk = (sq == 63) ? 56 : 63;
                    b[wk] = 'K'; b[bk] = 'k';
                    const std::uint64_t h0 = hash_of(b, true);
                    b[sq] = CH[c][pt];
                    Z[c][pt][sq] = hash_of(b, true) ^ h0;
                }
            // rey: claves relativas respecto a s0 (solo importan las diferencias por pares)
            const int s0 = 27;
            static const int cand[] = {0, 7, 56, 63, 3, 59};
            for (int s = 0; s < 64; ++s) {
                if (s == s0) { Z[c][5][s] = 0; continue; }
                int X = -1;
                for (int cx : cand)
                    if (cheb(cx, s) >= 2 && cheb(cx, s0) >= 2) { X = cx; break; }
                clear_b();
                b[X] = CH[1 - c][5];
                b[s] = CH[c][5];
                const std::uint64_t h1 = hash_of(b, true);
                b[s] = '.';
                b[s0] = CH[c][5];
                Z[c][5][s] = h1 ^ hash_of(b, true);
            }
        }

        std::memset(cuckoo, 0, sizeof(cuckoo));
        std::memset(cuckooMove, 0, sizeof(cuckooMove));
        cuckooCount = 0;
        for (int c = 0; c < 2; ++c)
            for (int pt = 1; pt <= 5; ++pt)
                for (int s1 = 0; s1 < 64; ++s1)
                    for (int s2 = s1 + 1; s2 < 64; ++s2) {
                        std::uint64_t att = 0;
                        switch (pt) {
                            case 1: att = bb_knight(s1); break;
                            case 2: att = bb_bishop(s1, 0); break;
                            case 3: att = bb_rook(s1, 0); break;
                            case 4: att = bb_bishop(s1, 0) | bb_rook(s1, 0); break;
                            default: att = bb_king(s1); break;
                        }
                        if (!((att >> s2) & 1)) continue;
                        std::uint16_t mv  = static_cast<std::uint16_t>((s1 << 6) | s2);
                        std::uint64_t key = Z[c][pt][s1] ^ Z[c][pt][s2] ^ zside;
                        std::uint32_t i   = H1(key);
                        while (true) {
                            std::swap(cuckoo[i], key);
                            std::swap(cuckooMove[i], mv);
                            if (mv == 0) break;
                            i = (i == H1(key)) ? H2(key) : H1(key);
                        }
                        ++cuckooCount;
                    }
    }
};

inline const Tables& tables() {
    static const Tables t;
    return t;
}

// ============================================================================
//  SEE (con pins, como Position::see_ge de SF)
// ============================================================================
inline std::uint64_t attackers_to(const chess::Board& pos, int sq, std::uint64_t occ) {
    using PT = chess::PieceType;
    return (bb_pawn(1, sq) & pcs(pos, PT::PAWN, 0)) | (bb_pawn(0, sq) & pcs(pos, PT::PAWN, 1)) |
           (bb_knight(sq) & pcs(pos, PT::KNIGHT)) | (bb_king(sq) & pcs(pos, PT::KING)) |
           (bb_bishop(sq, occ) & (pcs(pos, PT::BISHOP) | pcs(pos, PT::QUEEN))) |
           (bb_rook(sq, occ) & (pcs(pos, PT::ROOK) | pcs(pos, PT::QUEEN)));
}

// Piezas (de cualquier color) que bloquean un rayo hacia el rey de 'c'; 'pinners' = atacantes de las clavadas reales
inline std::uint64_t slider_blockers(const chess::Board& pos, int c, std::uint64_t& pinners, const Tables& T) {
    using PT = chess::PieceType;
    const int ksq = pos.kingSq(chess::Color(c)).index();
    const std::uint64_t enemy = bits(pos.us(chess::Color(c ^ 1)));
    const std::uint64_t q = pcs(pos, PT::QUEEN);
    std::uint64_t snipers = ((bb_rook(ksq, 0) & (pcs(pos, PT::ROOK) | q)) |
                             (bb_bishop(ksq, 0) & (pcs(pos, PT::BISHOP) | q))) & enemy;
    const std::uint64_t occupancy = bits(pos.occ()) ^ snipers;
    const std::uint64_t own = bits(pos.us(chess::Color(c)));
    std::uint64_t blockers = 0;
    pinners = 0;
    while (snipers) {
        const int s = pop_lsb64(snipers);
        const std::uint64_t b = T.between[ksq][s] & occupancy;
        if (b && !more_than_one(b)) {
            blockers |= b;
            if (b & own) pinners |= 1ULL << s;
        }
    }
    return blockers;
}

inline bool see_ge(const chess::Board& pos, Move m, int threshold) {
    using PT = chess::PieceType;
    if (m.typeOf() != Move::NORMAL) return VALUE_ZERO >= threshold;

    const int from = mv_from(m), to = mv_to(m);
    const chess::Piece pf = pos.at(chess::Square(from));
    int swap = PIECE_VALUE[pt_idx(pos.at<chess::PieceType>(chess::Square(to)))] - threshold;
    if (swap < 0) return false;
    swap = PIECE_VALUE[pt_idx(pf.type())] - swap;
    if (swap <= 0) return true;

    const Tables& T = tables();
    std::uint64_t occupied = bits(pos.occ()) ^ (1ULL << from) ^ (1ULL << to);
    int stm = static_cast<int>(pf.color());
    std::uint64_t attackers = attackers_to(pos, to, occupied);
    const std::uint64_t P = pcs(pos, PT::PAWN), N = pcs(pos, PT::KNIGHT), B = pcs(pos, PT::BISHOP),
                        R = pcs(pos, PT::ROOK), Q = pcs(pos, PT::QUEEN);
    const std::uint64_t col[2] = {bits(pos.us(chess::Color(0))), bits(pos.us(chess::Color(1)))};
    std::uint64_t blk[2] = {0, 0}, pin[2] = {0, 0};
    bool pinDone[2] = {false, false};
    std::uint64_t stmAttackers, bb;
    int res = 1;

    while (true) {
        stm ^= 1;
        attackers &= occupied;
        if (!(stmAttackers = attackers & col[stm])) break;

        if (!pinDone[stm]) {
            // pinners(~stm) se calculan respecto al rey de stm
            blk[stm] = slider_blockers(pos, stm, pin[stm], T);
            pinDone[stm] = true;
        }
        if (pin[stm] & occupied) {
            stmAttackers &= ~blk[stm];
            if (!stmAttackers) break;
        }

        res ^= 1;

        if ((bb = stmAttackers & P)) {
            if ((swap = PIECE_VALUE[0] - swap) < res) break;
            occupied ^= bb & (~bb + 1);
            attackers |= bb_bishop(to, occupied) & (B | Q);
        } else if ((bb = stmAttackers & N)) {
            if ((swap = PIECE_VALUE[1] - swap) < res) break;
            occupied ^= bb & (~bb + 1);
        } else if ((bb = stmAttackers & B)) {
            if ((swap = PIECE_VALUE[2] - swap) < res) break;
            occupied ^= bb & (~bb + 1);
            attackers |= bb_bishop(to, occupied) & (B | Q);
        } else if ((bb = stmAttackers & R)) {
            if ((swap = PIECE_VALUE[3] - swap) < res) break;
            occupied ^= bb & (~bb + 1);
            attackers |= bb_rook(to, occupied) & (R | Q);
        } else if ((bb = stmAttackers & Q)) {
            if ((swap = PIECE_VALUE[4] - swap) < res) break;
            occupied ^= bb & (~bb + 1);
            attackers |= (bb_bishop(to, occupied) & (B | Q)) | (bb_rook(to, occupied) & (R | Q));
        } else  // rey: si el rival aun tiene atacantes la jugada era ilegal
            return (attackers & ~col[stm]) ? res ^ 1 : res;
    }
    return static_cast<bool>(res);
}

// ============================================================================
//  REPETICIONES (usa pos_history; ss->pliesFromNull limita el barrido como en SF)
// ============================================================================
FB_INLINE bool is_draw(const ThreadWorker& w, const chess::Board& pos, int ply, int pliesFromNull) {
    const int hfm = static_cast<int>(pos.halfMoveClock());
    if (hfm > 99) {
        if (!pos.inCheck() || chess::movegen::anylegalmoves(pos)) return true;
    }
    const auto& h = w.pos_history;
    const int n = static_cast<int>(h.size());
    const int end = std::min(std::min(hfm, pliesFromNull), n - 1);
    if (end < 4) return false;
    const std::uint64_t key = h[n - 1];
    int cnt = 0;
    for (int i = 4; i <= end; i += 2)
        if (h[n - 1 - i] == key) {
            if (i < ply) return true;      // repite una vez dentro del arbol
            if (++cnt >= 2) return true;   // triple repeticion
        }
    return false;
}

inline bool upcoming_repetition(const ThreadWorker& w, const chess::Board& pos, int ply, int pliesFromNull,
                                const Tables& T) {
    const auto& h = w.pos_history;
    const int n = static_cast<int>(h.size());
    const int hfm = static_cast<int>(pos.halfMoveClock());
    const int end = std::min(std::min(hfm, pliesFromNull), n - 1);
    if (end < 3) return false;

    const std::uint64_t originalKey = h[n - 1];
    std::uint64_t other = originalKey ^ h[n - 2] ^ T.zside;

    for (int i = 3; i <= end; i += 2) {
        other ^= h[n - i] ^ h[n - 1 - i] ^ T.zside;
        if (other != 0) continue;
        const std::uint64_t moveKey = originalKey ^ h[n - 1 - i];
        int j = static_cast<int>(Tables::H1(moveKey));
        if (T.cuckoo[j] != moveKey) {
            j = static_cast<int>(Tables::H2(moveKey));
            if (T.cuckoo[j] != moveKey) continue;
        }
        const int s1 = (T.cuckooMove[j] >> 6) & 63, s2 = T.cuckooMove[j] & 63;
        if (!(T.between[s1][s2] & bits(pos.occ()))) {
            if (ply > i) return true;
            // antes o en la raiz: comprobar que la posicion i-atras ya era una repeticion
            const std::uint64_t k2 = h[n - 1 - i];
            const int lim = std::max(0, hfm - i);
            for (int d = 4; d <= lim && n - 1 - i - d >= 0; d += 2)
                if (h[n - 1 - i - d] == k2) return true;
        }
    }
    return false;
}

// ============================================================================
//  EVALUACION INCREMENTAL (espejo de rubi_push/rubi_pop/evaluate, pero con el
//  contexto cacheado en el worker: evita el TLS emulado de MinGW en cada nodo)
// ============================================================================
FB_INLINE bool eval_push(RubiEvalContext& c, std::uint64_t parentHash, Move m) {
    if (FB_UNLIKELY(!c.synchronized || c.position.hash() != parentHash || c.move_count >= 512)) return false;
    if (FB_UNLIKELY(!Stockfish::Probe::push(c.position, m))) {
        c.synchronized = false;
        return false;
    }
    c.position.makeMove(m);
    c.move_stack[c.move_count++] = m;
    return true;
}

// OPCIONAL: null move incremental. Requiere que sfdev_probe.h ofrezca
//     bool Stockfish::Probe::push_null(const chess::Board& before);   // y pop() lo deshace
// y compilar con -DFB_PROBE_HAS_NULL. Sin eso, cada null move fuerza un resync por FEN.
#ifdef FB_PROBE_HAS_NULL
FB_INLINE bool eval_push_null(RubiEvalContext& c, std::uint64_t parentHash) {
    if (FB_UNLIKELY(!c.synchronized || c.position.hash() != parentHash || c.move_count >= 512)) return false;
    if (FB_UNLIKELY(!Stockfish::Probe::push_null(c.position))) {
        c.synchronized = false;
        return false;
    }
    c.position.makeNullMove();
    c.move_stack[c.move_count++] = Move(static_cast<std::uint16_t>(Move::NULL_MOVE));
    return true;
}
#endif

// childHash = hash de la posicion hija (antes de deshacer la jugada en el tablero de busqueda)
FB_INLINE void eval_pop(RubiEvalContext& c, std::uint64_t childHash) {
    if (FB_LIKELY(c.synchronized && c.move_count > 0 && c.position.hash() == childHash)) {
        Stockfish::Probe::pop();
        const Move mv = c.move_stack[--c.move_count];
        if (mv.move() == Move::NULL_MOVE) c.position.unmakeNullMove();
        else c.position.unmakeMove(mv);
    }
    // si no coincide, el contexto ya fue resincronizado mas abajo: se resincroniza perezosamente
}

FB_INLINE int node_optimism(const ThreadWorker& w, chess::Color stm) {
    return stm == w.root_stm ? w.optimism : -w.optimism;
}

FB_INLINE Value evaluate_pos(ThreadWorker& w, const chess::Board& pos) {
    RubiEvalContext& c = *w.ctx;
    if (FB_UNLIKELY(!c.synchronized || c.position.hash() != pos.hash())) rubi_sync(pos);
    const int v = static_cast<int>(Stockfish::Probe::eval_current(node_optimism(w, pos.sideToMove())).value);
    return std::clamp(v, VALUE_TB_LOSS_IN_MAX_PLY + 1, VALUE_TB_WIN_IN_MAX_PLY - 1);
}

// ============================================================================
//  GESTION DE TIEMPO
// ============================================================================
constexpr double TM_OPTIMUM_FRACTION = 0.65;   // optimo = fraccion del limite duro (max_time_ms)

struct TimeState {
    std::atomic<long long> hard_limit_ms{0};   // 0 = sin limite
    std::chrono::high_resolution_clock::time_point limit_start{};
    std::atomic<bool> ponder_active{false};
};
inline TimeState g_time;

FB_INLINE long long limit_elapsed_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() -
                                                                 g_time.limit_start).count();
}

inline void apply_ponderhit() {
    std::lock_guard<std::mutex> lk(search_time_mutex);
    if (g_time.ponder_active.load()) {
        g_time.limit_start = std::chrono::high_resolution_clock::now();
        g_time.ponder_active.store(false);
    }
}

// Solo el hilo principal. Cada 512 llamadas comprueba reloj y ponderhit.
FB_INLINE void check_time(ThreadWorker& w) {
    if (--w.callsCnt > 0) return;
    w.callsCnt = 512;
    if (g_time.ponder_active.load(std::memory_order_relaxed)) {
        if (ponderhit_requested.load(std::memory_order_relaxed)) apply_ponderhit();
        else return;
    }
    const long long lim = g_time.hard_limit_ms.load(std::memory_order_relaxed);
    if (lim > 0 && limit_elapsed_ms() >= lim) stop_search.store(true, std::memory_order_relaxed);
}

}  // namespace fb

inline std::chrono::high_resolution_clock::time_point search_start_time_snapshot() {
    std::lock_guard<std::mutex> lk(search_time_mutex);
    return search_start_time;
}

inline void init_search_time(int max_time_ms, bool infinite, bool ponder) {
    {
        std::lock_guard<std::mutex> lk(search_time_mutex);
        const auto now = std::chrono::high_resolution_clock::now();
        search_start_time = now;
        fb::g_time.limit_start = now;
        fb::g_time.hard_limit_ms.store((infinite || max_time_ms <= 0) ? 0 : max_time_ms);
        fb::g_time.ponder_active.store(ponder);
        search_uses_ponder.store(ponder);
    }
    fb::tt_instance().new_search();
}

inline void wait_for_ponderhit() {
    while (!ponderhit_requested.load(std::memory_order_relaxed) && !stop_search.load(std::memory_order_relaxed))
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    if (ponderhit_requested.load(std::memory_order_relaxed)) fb::apply_ponderhit();
}

// ============================================================================
//  MOVE PICKER (etapas y scoring de Stockfish; movegen legal de chesslib)
//    - "Capturas" = capturas + ep + promociones a dama (con o sin captura)
//    - "Quietos"  = resto (incluye subpromociones y enroques)
// ============================================================================
namespace fb {

class MovePicker {
public:
    enum Stage : int {
        MAIN_TT, CAPTURE_INIT, GOOD_CAPTURE, QUIET_INIT, GOOD_QUIET, BAD_CAPTURE, BAD_QUIET,
        EVASION_TT, EVASION_INIT, EVASION,
        PROBCUT_TT, PROBCUT_INIT, PROBCUT,
        QSEARCH_TT, QCAPTURE_INIT, QCAPTURE
    };

    // Busqueda principal / qsearch / evasiones
    MovePicker(const chess::Board& p, Move ttm, int d, const WorkerData* wd, PieceToHist* const* ch, int pl,
               bool inCheck, ExtMove* buf, std::uint32_t pIdx)
        : pos(p), data(wd), contHist(ch), moves(buf), ttMove(ttm), depth(d), ply(pl), pawnIdx(pIdx) {
        const bool valid = ttm.move() && pos.isLegal(ttm);
        if (inCheck)
            stage = EVASION_TT + !valid;
        else if (d > 0)
            stage = MAIN_TT + !valid;
        else
            stage = QSEARCH_TT + !(valid && capture_stage(pos, ttm));
    }

    // ProbCut: solo capturas con SEE >= threshold
    MovePicker(const chess::Board& p, Move ttm, int thr, const WorkerData* wd, ExtMove* buf)
        : pos(p), data(wd), contHist(nullptr), moves(buf), ttMove(ttm), threshold(thr) {
        stage = PROBCUT_TT + !(ttm.move() && capture_stage(pos, ttm) && pos.isLegal(ttm));
    }

    Move next_move(bool skipQuiets = false) {
        for (;;) {
            switch (stage) {
                case MAIN_TT:
                case EVASION_TT:
                case QSEARCH_TT:
                case PROBCUT_TT:
                    ++stage;
                    return ttMove;

                case CAPTURE_INIT:
                case PROBCUT_INIT:
                case QCAPTURE_INIT:
                    cur = endBadCaptures = moves;
                    endCur = gen_captures(moves);
                    score_captures();
                    partial_insertion_sort(cur, endCur, INT32_MIN);
                    ++stage;
                    continue;

                case GOOD_CAPTURE: {
                    const Move m = select([&] {
                        // Las capturas perdedoras se apartan para el final
                        return see_ge(pos, cur->move, -cur->value / 18) ? true : (*endBadCaptures++ = *cur, false);
                    });
                    if (m.move()) return m;
                    ++stage;
                    continue;
                }

                case QUIET_INIT:
                    if (!skipQuiets) {
                        cur    = endBadQuiets = endBadCaptures;
                        endCur = gen_quiets(cur);
                        score_quiets();
                        partial_insertion_sort(cur, endCur, quiet_threshold(depth));
                        endBadQuiets   = endCur;
                        beginBadQuiets = endCur;
                    } else {
                        beginBadQuiets = endBadQuiets = cur = endCur = endBadCaptures;
                    }
                    ++stage;
                    continue;

                case GOOD_QUIET:
                    if (!skipQuiets) {
                        const Move m = select([] { return true; });
                        if (m.move()) {
                            if ((cur - 1)->value > -7998 || (cur - 1)->value <= quiet_threshold(depth)) return m;
                            // Los quiets restantes son malos
                            beginBadQuiets = cur - 1;
                        }
                    }
                    cur    = moves;
                    endCur = endBadCaptures;
                    ++stage;
                    continue;

                case BAD_CAPTURE: {
                    const Move m = select([] { return true; });
                    if (m.move()) return m;
                    cur    = beginBadQuiets;
                    endCur = endBadQuiets;
                    ++stage;
                    continue;
                }

                case BAD_QUIET:
                    if (!skipQuiets) return select([] { return true; });
                    return make_none();

                case EVASION_INIT:
                    cur    = moves;
                    endCur = gen_evasions(moves);
                    score_evasions();
                    partial_insertion_sort(cur, endCur, INT32_MIN);
                    ++stage;
                    continue;

                case EVASION:
                case QCAPTURE:
                    return select([] { return true; });

                case PROBCUT:
                    return select([&] { return see_ge(pos, cur->move, threshold); });
            }
            return make_none();
        }
    }

private:
    static constexpr int quiet_threshold(int d) { return -3560 * d; }

    template<typename Pred>
    Move select(Pred filter) {
        for (; cur < endCur; ++cur)
            if (cur->move != ttMove && filter()) return (cur++)->move;
        return make_none();
    }

    static void partial_insertion_sort(ExtMove* begin, ExtMove* end, int limit) {
        for (ExtMove *sortedEnd = begin, *p = begin + 1; p < end; ++p)
            if (p->value >= limit) {
                ExtMove tmp = *p, *q;
                *p = *++sortedEnd;
                for (q = sortedEnd; q != begin && (q - 1)->value < tmp.value; --q) *q = *(q - 1);
                *q = tmp;
            }
    }

    static bool is_underpromo(Move m) {
        return m.typeOf() == Move::PROMOTION && m.promotionType() != chess::PieceType::QUEEN;
    }

    bool pawns_on_seventh() const {
        const int us = static_cast<int>(pos.sideToMove());
        const std::uint64_t rank7 = us == 0 ? 0x00FF000000000000ULL : 0x000000000000FF00ULL;
        return pcs(pos, chess::PieceType::PAWN, us) & rank7;
    }

    // ---- generacion ----
    ExtMove* gen_captures(ExtMove* out) {
        using MG = chess::movegen::MoveGenType;
        chess::Movelist ml;
        chess::movegen::legalmoves<MG::CAPTURE>(ml, pos);
        for (const Move& m : ml)
            if (!is_underpromo(m)) *out++ = ExtMove{m, 0};
        if (pawns_on_seventh()) {  // promociones a dama sin captura
            chess::Movelist pm;
            chess::movegen::legalmoves<MG::QUIET>(pm, pos, chess::PieceGenType::PAWN);
            for (const Move& m : pm)
                if (m.typeOf() == Move::PROMOTION && m.promotionType() == chess::PieceType::QUEEN)
                    *out++ = ExtMove{m, 0};
        }
        return out;
    }

    ExtMove* gen_quiets(ExtMove* out) {
        using MG = chess::movegen::MoveGenType;
        chess::Movelist ml;
        chess::movegen::legalmoves<MG::QUIET>(ml, pos);
        for (const Move& m : ml)
            if (!(m.typeOf() == Move::PROMOTION && m.promotionType() == chess::PieceType::QUEEN))
                *out++ = ExtMove{m, 0};
        if (pawns_on_seventh()) {  // subpromociones con captura
            chess::Movelist pm;
            chess::movegen::legalmoves<MG::CAPTURE>(pm, pos, chess::PieceGenType::PAWN);
            for (const Move& m : pm)
                if (is_underpromo(m)) *out++ = ExtMove{m, 0};
        }
        return out;
    }

    ExtMove* gen_evasions(ExtMove* out) {
        chess::Movelist ml;
        chess::movegen::legalmoves<chess::movegen::MoveGenType::ALL>(ml, pos);
        for (const Move& m : ml) *out++ = ExtMove{m, 0};
        return out;
    }

    // ---- scoring ----
    void score_captures() {
        for (ExtMove* it = cur; it < endCur; ++it) {
            const Move m = it->move;
            const int  pc = pc_idx(pos.at(chess::Square(mv_from(m))));
            const int  to = mv_to(m);
            const int  cpt = pt_idx(pos.at<chess::PieceType>(chess::Square(to)));
            it->value = data->captureHistory[pc][to][cpt] + 7 * PIECE_VALUE[cpt];
        }
    }

    void score_quiets() {
        using PT = chess::PieceType;
        static constexpr int BONUS[6] = {0, 144, 144, 256, 517, 10000};
        const int us = static_cast<int>(pos.sideToMove()), them = us ^ 1;
        const std::uint64_t occ = bits(pos.occ());

        // casillas desde las que una pieza nuestra da jaque
        const int ksq = pos.kingSq(chess::Color(them)).index();
        std::uint64_t chk[6];
        chk[0] = bb_pawn(them, ksq);
        chk[1] = bb_knight(ksq);
        chk[2] = bb_bishop(ksq, occ);
        chk[3] = bb_rook(ksq, occ);
        chk[4] = chk[2] | chk[3];
        chk[5] = 0;

        // ataques enemigos
        const std::uint64_t theirP = pcs(pos, PT::PAWN, them);
        const std::uint64_t byPawn =
            them == 0 ? bits(chess::attacks::pawnLeftAttacks<chess::Color::WHITE>(chess::Bitboard(theirP)) |
                             chess::attacks::pawnRightAttacks<chess::Color::WHITE>(chess::Bitboard(theirP)))
                      : bits(chess::attacks::pawnLeftAttacks<chess::Color::BLACK>(chess::Bitboard(theirP)) |
                             chess::attacks::pawnRightAttacks<chess::Color::BLACK>(chess::Bitboard(theirP)));
        std::uint64_t byMinor = byPawn, byRook = 0, t;
        t = pcs(pos, PT::KNIGHT, them);
        while (t) byMinor |= bb_knight(pop_lsb64(t));
        t = pcs(pos, PT::BISHOP, them);
        while (t) byMinor |= bb_bishop(pop_lsb64(t), occ);
        byRook = byMinor;
        t = pcs(pos, PT::ROOK, them);
        while (t) byRook |= bb_rook(pop_lsb64(t), occ);
        const std::uint64_t threatened = (pcs(pos, PT::QUEEN, us) & byRook) | (pcs(pos, PT::ROOK, us) & byMinor) |
                                         ((pcs(pos, PT::KNIGHT, us) | pcs(pos, PT::BISHOP, us)) & byPawn);

        for (ExtMove* it = cur; it < endCur; ++it) {
            const Move m = it->move;
            const int from = mv_from(m), to = mv_to(m), ft = mv_from_to(m);
            const chess::Piece pcP = pos.at(chess::Square(from));
            const int pc = pc_idx(pcP), pt = pt_idx(pcP.type()), pt_to = pc * 64 + to;

            int v = 2 * data->mainHistory[us][ft];
            v += 2 * data->pawnHistory[pawnIdx][pt_to];
            v += contHist[0]->v[pt_to];
            v += contHist[1]->v[pt_to];
            v += contHist[2]->v[pt_to];
            v += contHist[3]->v[pt_to];
            v += contHist[5]->v[pt_to];

            // bonus por jaque (con SEE razonable)
            if (((chk[pt] >> to) & 1) && see_ge(pos, m, -75)) v += 16384;

            // penalizacion por ir a casilla amenazada por pieza menor / bonus por escapar
            const int tv = ((byPawn >> to) & 1) ? -95 : 100 * static_cast<int>((threatened >> from) & 1);
            v += BONUS[pt] * tv;

            if (ply < LOW_PLY_HISTORY_SIZE) v += 8 * data->lowPlyHistory[ply][ft] / (1 + ply);
            it->value = v;
        }
    }

    void score_evasions() {
        const int us = static_cast<int>(pos.sideToMove());
        for (ExtMove* it = cur; it < endCur; ++it) {
            const Move m = it->move;
            const int  to = mv_to(m), ft = mv_from_to(m);
            if (capture_stage(pos, m)) {
                const int cpt = pt_idx(pos.at<chess::PieceType>(chess::Square(to)));
                it->value = PIECE_VALUE[cpt] + (1 << 28);
            } else {
                const int pc = pc_idx(pos.at(chess::Square(mv_from(m))));
                it->value = data->mainHistory[us][ft] + contHist[0]->v[pc * 64 + to];
            }
        }
    }

    const chess::Board&      pos;
    const WorkerData*        data;
    PieceToHist* const*      contHist;
    ExtMove*                 moves;
    ExtMove *cur = nullptr, *endCur = nullptr, *endBadCaptures = nullptr, *beginBadQuiets = nullptr,
            *endBadQuiets = nullptr;
    Move          ttMove;
    int           stage     = 0;
    int           threshold = 0;
    int           depth     = 0;
    int           ply       = 0;
    std::uint32_t pawnIdx   = 0;
};

}  // namespace fb

// ============================================================================
//  BUSQUEDA
// ============================================================================
namespace fb {

enum NodeType { NonPV, PV, Root };

inline const std::array<int, 256>& reductions_table() {
    static const std::array<int, 256> t = [] {
        std::array<int, 256> a{};
        for (int i = 1; i < 256; ++i) a[i] = static_cast<int>(19.43 * std::log(i));
        return a;
    }();
    return t;
}

FB_INLINE Value value_draw(std::uint64_t nodes) { return VALUE_DRAW - 1 + static_cast<Value>(nodes & 0x2); }

FB_INLINE int reduction(const std::array<int, 256>& R, bool improving, Depth d, int mn, int delta, int rootDelta) {
    const int reductionScale = R[d] * R[mn];
    return reductionScale - delta * 735 / rootDelta + !improving * reductionScale * 191 / 512 + 1132;
}

FB_INLINE int futility_margin(Depth d, bool noTtCutNode, bool improving, bool oppWorsening) {
    const int futilityMult       = 112 - 26 * noTtCutNode;
    const int improvingDeduction = improving * futilityMult * 2;
    const int worseningDeduction = oppWorsening * futilityMult / 3;
    return futilityMult * d - improvingDeduction - worseningDeduction;
}

FB_INLINE int futility_move_count(bool improving, Depth depth) {
    return (3 + depth * depth) / (2 - improving);
}

FB_INLINE void update_pv(ThreadWorker& w, int ply, Move move) {
    Move* pv = w.pv_table[ply];
    const Move* child = w.pv_table[ply + 1];
    const int cl = w.pv_length[ply + 1];
    pv[0] = move;
    for (int i = 0; i < cl; ++i) pv[i + 1] = child[i];
    w.pv_length[ply] = cl + 1;
}

// ---- historiales ----
inline void update_continuation_histories(Stack* ss, int pc, int to, int bonus) {
    static constexpr int W[6] = {1108, 652, 273, 572, 126, 449};
    for (int i = 1; i <= 6; ++i) {
        if (ss->inCheck && i > 2) break;
        if (mv_ok((ss - i)->currentMove))
            stat_update<29952>((*(ss - i)->continuationHistory).v[pc * 64 + to], bonus * W[i - 1] / 1024 + 80 * (i < 2));
    }
}

inline void update_quiet_histories(WorkerData& d, const chess::Board& pos, Stack* ss, std::uint32_t pawnIdx, Move move,
                                   int bonus) {
    const int us = static_cast<int>(pos.sideToMove());
    const int ft = mv_from_to(move), to = mv_to(move);
    const int pc = pc_idx(pos.at(chess::Square(mv_from(move))));
    stat_update<7183>(d.mainHistory[us][ft], bonus);
    if (ss->ply < LOW_PLY_HISTORY_SIZE) stat_update<7183>(d.lowPlyHistory[ss->ply][ft], bonus * 741 / 1024);
    update_continuation_histories(ss, pc, to, bonus * (bonus > 0 ? 1018 : 1085) / 1024);
    stat_update<8192>(d.pawnHistory[pawnIdx][pc * 64 + to], bonus * (bonus > 0 ? 850 : 550) / 1024);
}

struct CorrKeys {
    std::uint32_t pawn, minor, npw, npb;
};
FB_INLINE CorrKeys corr_keys(const chess::Board& pos) {
    return CorrKeys{hidx(pawn_key(pos), 15), hidx(minor_key(pos), 15), hidx(nonpawn_key(pos, 0), 15),
                    hidx(nonpawn_key(pos, 1), 15)};
}

FB_INLINE int correction_value(const WorkerData& d, const CorrKeys& k, const chess::Board& pos, const Stack* ss) {
    const int  us  = static_cast<int>(pos.sideToMove());
    const Move m   = (ss - 1)->currentMove;
    const int pcv  = d.pawnCorrection[k.pawn][us];
    const int micv = d.minorCorrection[k.minor][us];
    const int wnp  = d.nonPawnCorrection[k.npw][0][us];
    const int bnp  = d.nonPawnCorrection[k.npb][1][us];
    int cntcv = 8;
    if (mv_ok(m)) {
        const int to  = mv_to(m);
        const int idx = pc_idx(pos.at(chess::Square(to))) * 64 + to;
        cntcv = (*(ss - 2)->continuationCorrectionHistory).v[idx] + (*(ss - 4)->continuationCorrectionHistory).v[idx];
    }
    return 9536 * pcv + 8494 * micv + 10132 * (wnp + bnp) + 7156 * cntcv;
}

FB_INLINE Value to_corrected_static_eval(Value v, int cv) {
    return std::clamp(v + cv / 131072, VALUE_TB_LOSS_IN_MAX_PLY + 1, VALUE_TB_WIN_IN_MAX_PLY - 1);
}

inline void update_correction_history(WorkerData& d, const CorrKeys& k, const chess::Board& pos, Stack* ss, int bonus) {
    const int  us = static_cast<int>(pos.sideToMove());
    const Move m  = (ss - 1)->currentMove;
    constexpr int nonPawnWeight = 165;
    stat_update<CORRECTION_HISTORY_LIMIT>(d.pawnCorrection[k.pawn][us], bonus);
    stat_update<CORRECTION_HISTORY_LIMIT>(d.minorCorrection[k.minor][us], bonus * 156 / 128);
    stat_update<CORRECTION_HISTORY_LIMIT>(d.nonPawnCorrection[k.npw][0][us], bonus * nonPawnWeight / 128);
    stat_update<CORRECTION_HISTORY_LIMIT>(d.nonPawnCorrection[k.npb][1][us], bonus * nonPawnWeight / 128);
    if (mv_ok(m)) {
        const int to  = mv_to(m);
        const int idx = pc_idx(pos.at(chess::Square(to))) * 64 + to;
        stat_update<CORRECTION_HISTORY_LIMIT>((*(ss - 2)->continuationCorrectionHistory).v[idx], bonus * 127 / 128);
        stat_update<CORRECTION_HISTORY_LIMIT>((*(ss - 4)->continuationCorrectionHistory).v[idx], bonus * 127 / 128);
    }
}

inline void update_all_stats(WorkerData& d, const chess::Board& pos, Stack* ss, std::uint32_t pawnIdx, Move bestMove,
                             Move ttMove, int prevSq, const Move* quietsSearched, int quietCount,
                             const Move* capturesSearched, int captureCount, Depth depth) {
    const int bonus = std::min(142 * depth - 88, 1501) + 318 * (bestMove == ttMove);
    const int malus = std::min(757 * depth - 172, 2848) - 30 * quietCount;

    if (!capture_stage(pos, bestMove)) {
        update_quiet_histories(d, pos, ss, pawnIdx, bestMove, bonus * 1054 / 1024);
        for (int i = 0; i < quietCount; ++i)
            update_quiet_histories(d, pos, ss, pawnIdx, quietsSearched[i], -malus * 1388 / 1024);
    } else {
        const int pc  = pc_idx(pos.at(chess::Square(mv_from(bestMove))));
        const int cpt = pt_idx(pos.at<chess::PieceType>(chess::Square(mv_to(bestMove))));
        stat_update<10692>(d.captureHistory[pc][mv_to(bestMove)][cpt], bonus * 1213 / 1024);
    }

    // Penalizacion extra al quiet temprano (no-TT) del ply anterior cuando se refuta
    if (prevSq >= 0 && (ss - 1)->moveCount == 1 + (ss - 1)->ttHit && (ss - 1)->capturedPt == PT_NONE)
        update_continuation_histories(ss - 1, pc_idx(pos.at(chess::Square(prevSq))), prevSq, -malus * 580 / 1024);

    for (int i = 0; i < captureCount; ++i) {
        const Move m  = capturesSearched[i];
        const int pc  = pc_idx(pos.at(chess::Square(mv_from(m))));
        const int cpt = pt_idx(pos.at<chess::PieceType>(chess::Square(mv_to(m))));
        stat_update<10692>(d.captureHistory[pc][mv_to(m)][cpt], -malus * 1428 / 1024);
    }
}

// ---- hacer / deshacer jugada (tablero + historial de hashes + NNUE incremental) ----
FB_INLINE bool do_move(ThreadWorker& w, chess::Board& pos, Stack* ss, Move m) {
    const bool pushed = eval_push(*w.ctx, pos.hash(), m);
    pos.makeMove(m);
    w.pos_history.push_back(pos.hash());
    (ss + 1)->pliesFromNull = ss->pliesFromNull + 1;
    return pushed;
}
FB_INLINE void undo_move(ThreadWorker& w, chess::Board& pos, Move m, bool pushed) {
    const std::uint64_t h = pos.hash();
    w.pos_history.pop_back();
    pos.unmakeMove(m);
    if (pushed) eval_pop(*w.ctx, h);
}

template<NodeType NT>
Value qsearch(ThreadWorker& w, chess::Board& pos, Stack* ss, Value alpha, Value beta);
template<NodeType NT>
Value search(ThreadWorker& w, chess::Board& pos, Stack* ss, Value alpha, Value beta, Depth depth, bool cutNode);

// ============================================================================
//  QSEARCH
// ============================================================================
template<NodeType NT>
Value qsearch(ThreadWorker& w, chess::Board& pos, Stack* ss, Value alpha, Value beta) {
    constexpr bool PvNode = NT == PV;
    static_assert(NT != Root, "qsearch no puede ser Root");

    if (alpha < VALUE_DRAW && upcoming_repetition(w, pos, ss->ply, ss->pliesFromNull, tables())) {
        alpha = value_draw(w.nodos_busqueda);
        if (alpha >= beta) return alpha;
    }

    WorkerData& d = *w.data;
    Move   bestMove = make_none(), move;
    Value  bestValue, value, futilityBase;
    bool   pvHit, givesCheck, capture;
    int    moveCount = 0;

    if (PvNode) w.pv_length[ss->ply] = 0;

    ss->inCheck = pos.inCheck();
    if (PvNode && w.sel_depth < ss->ply + 1) w.sel_depth = ss->ply + 1;

    if (stop_search.load(std::memory_order_relaxed)) throw SearchAbortedException();

    if (is_draw(w, pos, ss->ply, ss->pliesFromNull) || ss->ply >= MAX_PLY)
        return (ss->ply >= MAX_PLY && !ss->inCheck) ? evaluate_pos(w, pos) : VALUE_DRAW;

    // TT
    const Key posKey = pos.hash();
    TTProbe pr = w.tt->probe(posKey);
    const bool ttHit = pr.hit;
    TTData ttData = pr.data;
    TTEntry* ttWriter = pr.writer;
    ss->ttHit    = ttHit;
    ttData.move  = ttHit ? ttData.move : make_none();
    ttData.value = ttHit ? value_from_tt(ttData.value, ss->ply, static_cast<int>(pos.halfMoveClock())) : VALUE_NONE;
    pvHit        = ttHit && ttData.is_pv;

    if (!PvNode && ttData.depth >= DEPTH_QS && is_valid(ttData.value) &&
        (ttData.bound & (ttData.value >= beta ? BOUND_LOWER : BOUND_UPPER)))
        return ttData.value;

    // Evaluacion estatica
    Value unadjustedStaticEval = VALUE_NONE;
    if (ss->inCheck) {
        bestValue = futilityBase = -VALUE_INFINITE;
    } else {
        const CorrKeys ck = corr_keys(pos);
        const int correctionValue = correction_value(d, ck, pos, ss);
        if (ss->ttHit) {
            unadjustedStaticEval = ttData.eval;
            if (!is_valid(unadjustedStaticEval)) unadjustedStaticEval = evaluate_pos(w, pos);
            ss->staticEval = bestValue = to_corrected_static_eval(unadjustedStaticEval, correctionValue);
            if (is_valid(ttData.value) && !is_decisive(ttData.value) &&
                (ttData.bound & (ttData.value > bestValue ? BOUND_LOWER : BOUND_UPPER)))
                bestValue = ttData.value;
        } else {
            // Tras null move: eval previa con signo opuesto (evita un resync de NNUE)
            unadjustedStaticEval = (ss - 1)->currentMove.move() != Move::NULL_MOVE ? evaluate_pos(w, pos)
                                                                                    : -(ss - 1)->staticEval;
            ss->staticEval = bestValue = to_corrected_static_eval(unadjustedStaticEval, correctionValue);
        }

        // Stand pat
        if (bestValue >= beta) {
            if (!is_decisive(bestValue)) bestValue = (bestValue + beta) / 2;
            if (!ss->ttHit)
                tt_write(ttWriter, posKey, value_to_tt(bestValue, ss->ply), false, BOUND_LOWER, DEPTH_UNSEARCHED,
                         make_none(), unadjustedStaticEval, w.tt->generation());
            return bestValue;
        }
        if (bestValue > alpha) alpha = bestValue;
        futilityBase = ss->staticEval + 351;
    }

    PieceToHist* contHist[6] = {(ss - 1)->continuationHistory, (ss - 2)->continuationHistory, (ss - 3)->continuationHistory,
                                (ss - 4)->continuationHistory, nullptr, (ss - 6)->continuationHistory};
    const int prevSq = mv_ok((ss - 1)->currentMove) ? mv_to((ss - 1)->currentMove) : -1;
    const int us = static_cast<int>(pos.sideToMove());

    MovePicker mp(pos, ttData.move, DEPTH_QS, w.data, contHist, ss->ply, ss->inCheck, w.move_buf[0][ss->ply], 0);

    while ((move = mp.next_move()).move() != 0) {
        givesCheck = pos.givesCheck(move) != chess::CheckType::NO_CHECK;
        capture    = capture_stage(pos, move);
        ++moveCount;
        const int from = mv_from(move), to = mv_to(move);
        const chess::Piece movedPiece = pos.at(chess::Square(from));
        const int mpt = pc_idx(movedPiece) * 64 + to;

        // Pruning
        if (!is_loss(bestValue)) {
            if (!givesCheck && to != prevSq && !is_loss(futilityBase) && move.typeOf() != Move::PROMOTION) {
                if (moveCount > 2) continue;
                const Value futilityValue =
                    futilityBase + PIECE_VALUE[pt_idx(pos.at<chess::PieceType>(chess::Square(to)))];
                if (futilityValue <= alpha) {
                    bestValue = std::max(bestValue, futilityValue);
                    continue;
                }
                if (!see_ge(pos, move, alpha - futilityBase)) {
                    bestValue = std::min(alpha, futilityBase);
                    continue;
                }
            }
            if (!capture && contHist[0]->v[mpt] < 0 && contHist[1]->v[mpt] < 0) continue;
            if (!see_ge(pos, move, -78)) continue;
        }

        w.tt->prefetch(pos.zobristAfter<false>(move));

        ss->currentMove = move;
        ss->continuationHistory           = &d.continuationHistory[ss->inCheck][capture][mpt];
        ss->continuationCorrectionHistory = &d.continuationCorrection[mpt];
        ss->capturedPt = pt_idx(pos.getCapturing<chess::PieceType>(move));

        w.nodos_busqueda.inc();
        const bool pushed = do_move(w, pos, ss, move);
        value = -qsearch<NT>(w, pos, ss + 1, -beta, -alpha);
        undo_move(w, pos, move, pushed);

        if (value > bestValue) {
            bestValue = value;
            if (value > alpha) {
                bestMove = move;
                if (PvNode) update_pv(w, ss->ply, move);
                if (value < beta)
                    alpha = value;
                else
                    break;
            }
        }
    }

    // Mate
    if (bestValue == -VALUE_INFINITE) return mated_in(ss->ply);

    if (!is_decisive(bestValue) && bestValue > beta) bestValue = (bestValue + beta) / 2;

    // Caso especial de ahogado (solo rey + peones bloqueados tras capturar torre/dama)
    if (!ss->inCheck && !moveCount && !pos.hasNonPawnMaterial(chess::Color(us)) &&
        ((ss - 1)->capturedPt == pt_idx(chess::PieceType(chess::PieceType::ROOK)) ||
         (ss - 1)->capturedPt == pt_idx(chess::PieceType(chess::PieceType::QUEEN)))) {
        const std::uint64_t pawns = pcs(pos, chess::PieceType::PAWN, us);
        const std::uint64_t push  = (us == 0 ? pawns << 8 : pawns >> 8) & ~bits(pos.occ());
        if (!push && !chess::movegen::anylegalmoves(pos)) bestValue = VALUE_DRAW;
    }

    tt_write(ttWriter, posKey, value_to_tt(bestValue, ss->ply), pvHit, bestValue >= beta ? BOUND_LOWER : BOUND_UPPER,
             DEPTH_QS, bestMove, unadjustedStaticEval, w.tt->generation());
    return bestValue;
}

// ============================================================================
//  SEARCH
// ============================================================================
template<NodeType NT>
Value search(ThreadWorker& w, chess::Board& pos, Stack* ss, Value alpha, Value beta, Depth depth, bool cutNode) {
    constexpr bool PvNode   = NT != NonPV;
    constexpr bool rootNode = NT == Root;
    const bool     allNodes = !(PvNode || cutNode);

    if (depth <= 0) return qsearch<PvNode ? PV : NonPV>(w, pos, ss, alpha, beta);

    depth = std::min(depth, MAX_PLY - 1);

    if (!rootNode && alpha < VALUE_DRAW && upcoming_repetition(w, pos, ss->ply, ss->pliesFromNull, tables())) {
        alpha = value_draw(w.nodos_busqueda);
        if (alpha >= beta) return alpha;
    }

    WorkerData& d = *w.data;
    Move   move, bestMove = make_none();
    Depth  extension, newDepth;
    Value  bestValue, value, eval, probCutBeta;
    bool   givesCheck, improving = false, opponentWorsening = false, capture, moveCountPruning;
    int    moveCount, captureCount, quietCount;
    Move   capturesSearched[32], quietsSearched[32];

    // Step 1. Inicializar nodo
    ss->inCheck       = pos.inCheck();
    const bool priorCapture = (ss - 1)->capturedPt != PT_NONE;
    const int  us     = static_cast<int>(pos.sideToMove());
    moveCount = captureCount = quietCount = ss->moveCount = 0;
    bestValue = -VALUE_INFINITE;

    if (w.id == 0) check_time(w);

    if (PvNode && w.sel_depth < ss->ply + 1) w.sel_depth = ss->ply + 1;

    if (!rootNode) {
        // Step 2. Busqueda abortada o tablas inmediatas
        if (stop_search.load(std::memory_order_relaxed)) throw SearchAbortedException();
        if (is_draw(w, pos, ss->ply, ss->pliesFromNull) || ss->ply >= MAX_PLY)
            return (ss->ply >= MAX_PLY && !ss->inCheck) ? evaluate_pos(w, pos) : value_draw(w.nodos_busqueda);

        // Step 3. Mate distance pruning
        alpha = std::max(mated_in(ss->ply), alpha);
        beta  = std::min(mate_in(ss->ply + 1), beta);
        if (alpha >= beta) return alpha;
    } else {
        w.rootDelta = std::max(1, beta - alpha);
    }

    if (PvNode) w.pv_length[ss->ply] = 0;

    (ss + 2)->cutoffCnt = 0;
    const int prevSq = mv_ok((ss - 1)->currentMove) ? mv_to((ss - 1)->currentMove) : -1;
    ss->statScore = 0;

    const std::uint32_t pawnIdx = hidx(pawn_key(pos), 13);

    // Step 4. Consulta de la TT
    const Move excludedMove = ss->excludedMove;
    const Key  posKey       = pos.hash();
    TTProbe    pr           = w.tt->probe(posKey);
    const bool ttHit        = pr.hit;
    TTData     ttData       = pr.data;
    TTEntry*   ttWriter     = pr.writer;
    ss->ttHit    = ttHit;
    ttData.move  = rootNode ? w.root_moves[0].move : ttHit ? ttData.move : make_none();
    ttData.value = ttHit ? value_from_tt(ttData.value, ss->ply, static_cast<int>(pos.halfMoveClock())) : VALUE_NONE;
    ss->ttPv     = excludedMove.move() ? ss->ttPv : PvNode || (ttHit && ttData.is_pv);
    const bool ttCapture = ttData.move.move() && capture_stage(pos, ttData.move);

    // Corte temprano por TT en nodos no-PV
    if (!PvNode && !excludedMove.move() && ttData.depth > depth - (ttData.value <= beta) && is_valid(ttData.value) &&
        (ttData.bound & (ttData.value >= beta ? BOUND_LOWER : BOUND_UPPER)) &&
        (cutNode == (ttData.value >= beta) || depth > 9)) {
        if (ttData.move.move() && ttData.value >= beta) {
            if (!ttCapture)
                update_quiet_histories(d, pos, ss, pawnIdx, ttData.move, std::min(125 * depth - 77, 1157));
            if ((ss - 1)->moveCount <= 3 && !priorCapture && prevSq >= 0)
                update_continuation_histories(ss - 1, pc_idx(pos.at(chess::Square(prevSq))), prevSq, -2301);
        }

        // Workaround parcial del problema de interaccion con el historial: no cortar con rule50 alto
        if (pos.halfMoveClock() < 90) {
            if (depth >= 8 && ttData.move.move() && pos.isLegal(ttData.move) && !is_decisive(ttData.value)) {
                pos.makeMove(ttData.move);
                const Key nextKey = pos.hash();
                TTProbe   nx      = w.tt->probe(nextKey);
                pos.unmakeMove(ttData.move);
                // El valor de TT tras la jugada tambien debe provocar el corte
                if (!is_valid(nx.data.value)) return ttData.value;
                if ((ttData.value >= beta) == (-nx.data.value >= beta)) return ttData.value;
            } else
                return ttData.value;
        }
    }

    // Step 5. Tablebases: no implementado (SyzygyPath no esta soportado por el motor)

    // Step 6. Evaluacion estatica
    Value unadjustedStaticEval = VALUE_NONE;
    const CorrKeys ck = corr_keys(pos);
    const int correctionValue = correction_value(d, ck, pos, ss);

    if (ss->inCheck) {
        // Sin pruning temprano en jaque
        ss->staticEval = eval = VALUE_NONE;
        improving = false;
    } else {
        if (excludedMove.move()) {
            unadjustedStaticEval = eval = ss->staticEval;
        } else if (ss->ttHit) {
            unadjustedStaticEval = ttData.eval;
            if (!is_valid(unadjustedStaticEval)) unadjustedStaticEval = evaluate_pos(w, pos);
            ss->staticEval = eval = to_corrected_static_eval(unadjustedStaticEval, correctionValue);
            if (is_valid(ttData.value) && (ttData.bound & (ttData.value > eval ? BOUND_LOWER : BOUND_UPPER)))
                eval = ttData.value;
        } else {
            unadjustedStaticEval = evaluate_pos(w, pos);
            ss->staticEval = eval = to_corrected_static_eval(unadjustedStaticEval, correctionValue);
            tt_write(ttWriter, posKey, VALUE_NONE, ss->ttPv, BOUND_NONE, DEPTH_UNSEARCHED, make_none(),
                     unadjustedStaticEval, w.tt->generation());
        }

        // La diferencia de eval estatica mejora el orden de quiets
        if (mv_ok((ss - 1)->currentMove) && !(ss - 1)->inCheck && !priorCapture) {
            const int bonus = std::clamp(-10 * ((ss - 1)->staticEval + ss->staticEval), -1881, 1413) + 616;
            stat_update<7183>(d.mainHistory[us ^ 1][mv_from_to((ss - 1)->currentMove)], bonus * 1151 / 1024);
            const chess::Piece pp = pos.at(chess::Square(prevSq));
            if (pp.type() != chess::PieceType::PAWN && (ss - 1)->currentMove.typeOf() != Move::PROMOTION)
                stat_update<8192>(d.pawnHistory[pawnIdx][pc_idx(pp) * 64 + prevSq], bonus * 1107 / 1024);
        }

        improving         = ss->staticEval > (ss - 2)->staticEval;
        opponentWorsening = ss->staticEval + (ss - 1)->staticEval > 2;

        // Step 7. Razoring
        if (!PvNode && eval < alpha - 495 - 290 * depth * depth) {
            value = qsearch<NonPV>(w, pos, ss, alpha - 1, alpha);
            if (value < alpha && !is_decisive(value)) return value;
        }

        // Step 8. Futility pruning: nodo hijo
        if (!ss->ttPv && depth < 14 &&
            eval - futility_margin(depth, cutNode && !ss->ttHit, improving, opponentWorsening) -
                    (ss - 1)->statScore / 310 + 40 - std::abs(correctionValue) / 131072 >= beta &&
            eval >= beta && (!ttData.move.move() || ttCapture) && !is_loss(beta) && !is_win(eval))
            return beta + (eval - beta) / 3;

#if FB_NULL_MOVE_PRUNING
        // Step 9. Null move con busqueda de verificacion
        if (cutNode && (ss - 1)->currentMove.move() != Move::NULL_MOVE && eval >= beta &&
            ss->staticEval >= beta - 18 * depth + 390 && !excludedMove.move() && pos.hasNonPawnMaterial(chess::Color(us)) &&
            ss->ply >= w.nmpMinPly && !is_loss(beta)) {
            const Depth R = std::min((eval - beta) / 232, 6) + depth / 3 + 5;

            ss->currentMove                   = make_null();
            ss->continuationHistory           = &d.continuationHistory[0][0][PIECE_NONE_IDX * 64];
            ss->continuationCorrectionHistory = &d.continuationCorrection[PIECE_NONE_IDX * 64];
            ss->capturedPt                    = PT_NONE;

#ifdef FB_PROBE_HAS_NULL
            const bool nullPushed = eval_push_null(*w.ctx, pos.hash());
#endif
            pos.makeNullMove();
#ifdef FB_PROBE_HAS_NULL
            const std::uint64_t nullHash = pos.hash();
#endif
            (ss + 1)->pliesFromNull = 0;
            w.pos_history.push_back(pos.hash());
            const Value nullValue = -search<NonPV>(w, pos, ss + 1, -beta, -beta + 1, depth - R, false);
            w.pos_history.pop_back();
            pos.unmakeNullMove();
#ifdef FB_PROBE_HAS_NULL
            if (nullPushed) eval_pop(*w.ctx, nullHash);
#endif

            if (nullValue >= beta && !is_win(nullValue)) {
                if (w.nmpMinPly || depth < 16) return nullValue;

                // Verificacion a gran profundidad con NMP desactivado hasta superar nmpMinPly
                w.nmpMinPly = ss->ply + 3 * (depth - R) / 4;
                const Value v = search<NonPV>(w, pos, ss, beta - 1, beta, depth - R, false);
                w.nmpMinPly = 0;
                if (v >= beta) return nullValue;
            }
        }
#endif

        // Step 10. Reducciones internas iterativas
        if (PvNode && !ttData.move.move()) depth -= 3;
        if (depth <= 0) return qsearch<PV>(w, pos, ss, alpha, beta);
        if (cutNode && depth >= 7 && (!ttData.move.move() || ttData.bound == BOUND_UPPER))
            depth -= 1 + !ttData.move.move();

        // Step 11. ProbCut
        probCutBeta = beta + 224 - 64 * improving;
        if (depth >= 3 && !is_decisive(beta) && !(is_valid(ttData.value) && ttData.value < probCutBeta)) {
            MovePicker mp(pos, ttData.move, probCutBeta - ss->staticEval, w.data, w.move_buf[excludedMove.move() ? 1 : 0][ss->ply]);
            while ((move = mp.next_move()).move() != 0) {
                if (move == excludedMove) continue;
                const int from = mv_from(move), to = mv_to(move);
                const chess::Piece movedPiece = pos.at(chess::Square(from));
                const int cpt = pt_idx(pos.at<chess::PieceType>(chess::Square(to)));
                const int mpt = pc_idx(movedPiece) * 64 + to;

                w.tt->prefetch(pos.zobristAfter<false>(move));

                ss->currentMove                   = move;
                ss->continuationHistory           = &d.continuationHistory[ss->inCheck][true][mpt];
                ss->continuationCorrectionHistory = &d.continuationCorrection[mpt];
                ss->capturedPt                    = pt_idx(pos.getCapturing<chess::PieceType>(move));

                w.nodos_busqueda.inc();
                const bool pushed = do_move(w, pos, ss, move);

                // qsearch preliminar para verificar la jugada
                value = -qsearch<NonPV>(w, pos, ss + 1, -probCutBeta, -probCutBeta + 1);
                if (value >= probCutBeta)
                    value = -search<NonPV>(w, pos, ss + 1, -probCutBeta, -probCutBeta + 1, depth - 4, !cutNode);

                undo_move(w, pos, move, pushed);

                if (value >= probCutBeta) {
                    stat_update<10692>(d.captureHistory[pc_idx(movedPiece)][to][cpt], 1300);
                    tt_write(ttWriter, posKey, value_to_tt(value, ss->ply), ss->ttPv, BOUND_LOWER, depth - 3, move,
                             unadjustedStaticEval, w.tt->generation());
                    return is_decisive(value) ? value : value - (probCutBeta - beta);
                }
            }
        }
    }

    // Step 12. Pequeña idea de ProbCut
    probCutBeta = beta + 417;
    if ((ttData.bound & BOUND_LOWER) && ttData.depth >= depth - 4 && ttData.value >= probCutBeta &&
        !is_decisive(beta) && is_valid(ttData.value) && !is_decisive(ttData.value))
        return probCutBeta;

    PieceToHist* contHist[6] = {(ss - 1)->continuationHistory, (ss - 2)->continuationHistory, (ss - 3)->continuationHistory,
                                (ss - 4)->continuationHistory, nullptr, (ss - 6)->continuationHistory};

    MovePicker mp(pos, ttData.move, depth, w.data, contHist, ss->ply, ss->inCheck, w.move_buf[excludedMove.move() ? 1 : 0][ss->ply], pawnIdx);

    value            = bestValue;
    moveCountPruning = false;
    const auto& REDUCTIONS = reductions_table();

    // Step 13. Bucle de jugadas
    while ((move = mp.next_move(moveCountPruning)).move() != 0) {
        if (move == excludedMove) continue;

        ss->moveCount = ++moveCount;
        if (PvNode) w.pv_length[ss->ply + 1] = 0;

        extension = 0;
        capture   = capture_stage(pos, move);
        const int from = mv_from(move), to = mv_to(move), ft = mv_from_to(move);
        const chess::Piece movedPiece = pos.at(chess::Square(from));
        const int mpt = pc_idx(movedPiece) * 64 + to;
        const int cptPre = pt_idx(pos.at<chess::PieceType>(chess::Square(to)));   // para historiales
        givesCheck = pos.givesCheck(move) != chess::CheckType::NO_CHECK;

        newDepth = depth - 1;
        const int delta = beta - alpha;
        int r = reduction(REDUCTIONS, improving, depth, moveCount, delta, w.rootDelta);
        if (ss->ttPv) r += 931;

        // Step 14. Pruning a poca profundidad
        if (!rootNode && pos.hasNonPawnMaterial(chess::Color(us)) && !is_loss(bestValue)) {
            if (!moveCountPruning) moveCountPruning = moveCount >= futility_move_count(improving, depth);

            int lmrDepth = newDepth - r / 1024;

            if (capture || givesCheck) {
                const int captHist = d.captureHistory[pc_idx(movedPiece)][to][cptPre];

                // Futility pruning de capturas
                if (!givesCheck && lmrDepth < 7 && !ss->inCheck) {
                    const Value futilityValue = ss->staticEval + 225 + 220 * lmrDepth + PIECE_VALUE[cptPre] + 131 * captHist / 1024;
                    if (futilityValue <= alpha) continue;
                }

                // SEE para capturas y jaques
                const int seeHist = std::clamp(captHist / 31, -137 * depth, 125 * depth);
                if (!see_ge(pos, move, -158 * depth - seeHist)) continue;
            } else {
                int history = contHist[0]->v[mpt] + contHist[1]->v[mpt] + d.pawnHistory[pawnIdx][mpt];

                // Pruning por continuation history
                if (history < -4107 * depth) continue;

                history += 68 * d.mainHistory[us][ft] / 32;
                lmrDepth += history / 3560;

                const Value futilityValue = ss->staticEval + (bestValue < ss->staticEval - 52 ? 140 : 83) + 116 * lmrDepth;

                // Futility pruning: nodo padre
                if (!ss->inCheck && lmrDepth < 12 && futilityValue <= alpha) {
                    if (bestValue <= futilityValue && !is_decisive(bestValue) && !is_win(futilityValue))
                        bestValue = futilityValue;
                    continue;
                }

                lmrDepth = std::max(lmrDepth, 0);

                // SEE negativo
                if (!see_ge(pos, move, -26 * lmrDepth * lmrDepth)) continue;
            }
        }

        // Step 15. Extensiones
        if (!rootNode && move == ttData.move && !excludedMove.move() &&
            depth >= 4 - (w.completedDepth > 35) + ss->ttPv && is_valid(ttData.value) && !is_decisive(ttData.value) &&
            (ttData.bound & BOUND_LOWER) && ttData.depth >= depth - 3) {
            const Value singularBeta  = ttData.value - (56 + 79 * (ss->ttPv && !PvNode)) * depth / 64;
            const Depth singularDepth = newDepth / 2;

            ss->excludedMove = move;
            value = search<NonPV>(w, pos, ss, singularBeta - 1, singularBeta, singularDepth, cutNode);
            ss->excludedMove = make_none();

            if (value < singularBeta) {
                const int corrValAdj1  = std::abs(correctionValue) / 248400;
                const int corrValAdj2  = std::abs(correctionValue) / 249757;
                const int doubleMargin = -4 + 244 * PvNode - 206 * !ttCapture - corrValAdj1 -
                                         997 * d.ttMoveHistory / 131072 - (ss->ply > w.rootDepth) * 47;
                const int tripleMargin = 84 + 269 * PvNode - 253 * !ttCapture + 91 * ss->ttPv - corrValAdj2 -
                                         (ss->ply * 2 > w.rootDepth * 3) * 54;

                extension = 1 + (value < singularBeta - doubleMargin) + (value < singularBeta - tripleMargin);
                depth += ((!PvNode) && (depth < 14));
            }
            // Multi-cut
            else if (value >= beta && !is_decisive(value))
                return value;
            // Extensiones negativas
            else if (ttData.value >= beta)
                extension = -3;
            else if (cutNode)
                extension = -2;
        }
        // Extension por recapturar la pieza que acaba de mover
        else if (PvNode && to == prevSq && d.captureHistory[pc_idx(movedPiece)][to][cptPre] > 4126)
            extension = 1;

        // Step 16. Hacer la jugada
        newDepth += extension;
        const std::uint64_t nodeCount = rootNode ? static_cast<std::uint64_t>(w.nodos_busqueda) : 0;

        w.tt->prefetch(pos.zobristAfter<false>(move));

        ss->currentMove                   = move;
        ss->continuationHistory           = &d.continuationHistory[ss->inCheck][capture][mpt];
        ss->continuationCorrectionHistory = &d.continuationCorrection[mpt];
        const int cptAfter = pt_idx(pos.getCapturing<chess::PieceType>(move));
        ss->capturedPt     = cptAfter;

        w.nodos_busqueda.inc();
        const bool pushed = do_move(w, pos, ss, move);

        // Ajustes de reduccion
        if (ss->ttPv)
            r -= 2719 + PvNode * 983 + (ttData.value > alpha) * 922 + (ttData.depth >= depth) * (934 + cutNode * 1011);
        r += 714;
        r -= moveCount * 73;
        r -= std::abs(correctionValue) / 30370;

        if (cutNode) r += 3372 + 997 * !ttData.move.move();
        if (ttCapture) r += 1119;
        if ((ss + 1)->cutoffCnt > 2) r += 991 + 1042 * allNodes;
        if (move == ttData.move) r -= 2151;

        if (capture)
            ss->statScore = 803 * PIECE_VALUE[cptAfter] / 128 + d.captureHistory[pc_idx(movedPiece)][to][cptPre] - 4585;
        else
            ss->statScore = 2 * d.mainHistory[us][ft] + contHist[0]->v[mpt] + contHist[1]->v[mpt] - 3206;

        r -= ss->statScore * 794 / 8192;

        // Step 17. Late Move Reductions
        if (depth >= 2 && moveCount > 1) {
            const Depth dd = std::max(1, std::min(newDepth - r / 1024, newDepth + !allNodes));

            ss->reduction = newDepth - dd;
            value = -search<NonPV>(w, pos, ss + 1, -(alpha + 1), -alpha, dd, true);
            ss->reduction = 0;

            // Busqueda a profundidad completa si la reducida supera alpha
            if (value > alpha && dd < newDepth) {
                const bool doDeeperSearch    = value > (bestValue + 43 + 2 * newDepth);
                const bool doShallowerSearch = value < bestValue + 9;
                newDepth += doDeeperSearch - doShallowerSearch;

                if (newDepth > dd) value = -search<NonPV>(w, pos, ss + 1, -(alpha + 1), -alpha, newDepth, !cutNode);

                const int bonus = (value >= beta) * 2010;
                update_continuation_histories(ss, pc_idx(movedPiece), to, bonus);
            }
        }
        // Step 18. Busqueda completa cuando se omite LMR
        else if (!PvNode || moveCount > 1) {
            if (!ttData.move.move()) r += 2111;
            value = -search<NonPV>(w, pos, ss + 1, -(alpha + 1), -alpha, newDepth - (r > 3444), !cutNode);
        }

        // Busqueda PV completa para la primera jugada o tras un fail-high
        if (PvNode && (moveCount == 1 || value > alpha)) {
            w.pv_length[ss->ply + 1] = 0;
            if (move == ttData.move && w.rootDepth > 8) newDepth = std::max(newDepth, 1);
            value = -search<PV>(w, pos, ss + 1, -beta, -alpha, newDepth, false);
        }

        // Step 19. Deshacer
        undo_move(w, pos, move, pushed);

        // Step 20. Nueva mejor jugada?
        if (stop_search.load(std::memory_order_relaxed)) throw SearchAbortedException();

        if (rootNode) {
            auto it = std::find_if(w.root_moves.begin(), w.root_moves.end(), [&](const RootMove& rm) { return rm.move == move; });
            if (it != w.root_moves.end()) {
                RootMove& rm = *it;
                rm.effort += static_cast<std::uint64_t>(w.nodos_busqueda) - nodeCount;
                rm.averageScore = rm.averageScore != -VALUE_INFINITE ? (value + rm.averageScore) / 2 : value;
                rm.meanSquaredScore = rm.meanSquaredScore != -VALUE_INFINITE * VALUE_INFINITE
                                          ? (value * std::abs(value) + rm.meanSquaredScore) / 2
                                          : value * std::abs(value);
                if (moveCount == 1 || value > alpha) {
                    rm.score = value;
                    if (moveCount > 1) ++w.bestMoveChanges;
                } else
                    rm.score = -VALUE_INFINITE;
            }
        }

        // Entre jugadas empatadas, promocionar una alternativa a mejor jugada
        const int inc = (value == bestValue && ss->ply + 2 >= w.rootDepth && (static_cast<int>(w.nodos_busqueda) & 14) == 0 &&
                         !is_win(std::abs(value) + 1)) ? 1 : 0;

        if (value + inc > bestValue) {
            bestValue = value;

            if (value + inc > alpha) {
                bestMove = move;

                if (PvNode) update_pv(w, ss->ply, move);

                if (value >= beta) {
                    ss->cutoffCnt += (extension < 2) || PvNode;
                    break;
                } else {
                    if (depth > 2 && depth < 14 && !is_decisive(value)) depth -= 2;
                    alpha = value;
                }
            }
        }

        if (move != bestMove && moveCount <= 32) {
            if (capture)
                capturesSearched[captureCount++] = move;
            else
                quietsSearched[quietCount++] = move;
        }
    }

    // Step 21. Mate y ahogado
    if (bestValue >= beta && !is_decisive(bestValue) && !is_decisive(alpha)) bestValue = (bestValue * depth + beta) / (depth + 1);

    if (!moveCount)
        bestValue = excludedMove.move() ? alpha : ss->inCheck ? mated_in(ss->ply) : VALUE_DRAW;
    else if (bestMove.move()) {
        update_all_stats(d, pos, ss, pawnIdx, bestMove, ttData.move, prevSq, quietsSearched, quietCount,
                         capturesSearched, captureCount, depth);
        if (!PvNode) stat_update<8192>(d.ttMoveHistory, bestMove == ttData.move ? 811 : -848);
    }
    // Bonus a la jugada previa tranquila que provoco el fail low
    else if (!priorCapture && prevSq >= 0) {
        int bonusScale = 118 * (depth > 5) + 36 * !allNodes + 161 * ((ss - 1)->moveCount > 8) +
                         133 * (!ss->inCheck && bestValue <= ss->staticEval - 107) +
                         120 * (!(ss - 1)->inCheck && bestValue <= -(ss - 1)->staticEval - 84);
        bonusScale += std::min(-(ss - 1)->statScore / 106, 318);
        bonusScale = std::max(bonusScale, 0);

        const int scaledBonus = std::min(160 * depth - 99, 1492) * bonusScale;
        const chess::Piece pp = pos.at(chess::Square(prevSq));

        update_continuation_histories(ss - 1, pc_idx(pp), prevSq, scaledBonus * 388 / 32768);
        stat_update<7183>(d.mainHistory[us ^ 1][mv_from_to((ss - 1)->currentMove)], scaledBonus * 212 / 32768);
        if (pp.type() != chess::PieceType::PAWN && (ss - 1)->currentMove.typeOf() != Move::PROMOTION)
            stat_update<8192>(d.pawnHistory[pawnIdx][pc_idx(pp) * 64 + prevSq], scaledBonus * 1055 / 32768);
    } else if (priorCapture && prevSq >= 0) {
        const chess::Piece pp = pos.at(chess::Square(prevSq));
        stat_update<10692>(d.captureHistory[pc_idx(pp)][prevSq][(ss - 1)->capturedPt], 1080);
    }

    // Si no hay buena jugada y la posicion previa era ttPv, se anade al arbol PV
    if (bestValue <= alpha) ss->ttPv = ss->ttPv || ((ss - 1)->ttPv && depth > 3);

    // Guardar en la TT (la eval estatica se guarda sin correccion)
    if (!excludedMove.move())
        tt_write(ttWriter, posKey, value_to_tt(bestValue, ss->ply), ss->ttPv,
                 bestValue >= beta ? BOUND_LOWER : (PvNode && bestMove.move()) ? BOUND_EXACT : BOUND_UPPER,
                 moveCount != 0 ? depth : std::min(MAX_PLY - 1, depth + 6), bestMove, unadjustedStaticEval,
                 w.tt->generation());

    // Correction history
    if (!ss->inCheck && (!bestMove.move() || !capture_stage(pos, bestMove)) &&
        !(bestValue >= beta && bestValue <= ss->staticEval) && !(!bestMove.move() && bestValue >= ss->staticEval)) {
        const int bonus = std::clamp((bestValue - ss->staticEval) * depth / 8, -CORRECTION_HISTORY_LIMIT / 4,
                                     CORRECTION_HISTORY_LIMIT / 4);
        update_correction_history(d, ck, pos, ss, bonus);
    }

    return bestValue;
}

}  // namespace fb

// ============================================================================
//  ENTRADA PUBLICA
// ============================================================================
namespace fb {

inline void init_worker_stack(ThreadWorker& w) {
    WorkerData& d = *w.data;
    for (int j = 0; j < MAX_PLY + 16; ++j) {
        Stack& s = w.stack[j];
        s.continuationHistory           = &d.continuationHistory[0][0][PIECE_NONE_IDX * 64];
        s.continuationCorrectionHistory = &d.continuationCorrection[PIECE_NONE_IDX * 64];
        s.currentMove   = make_none();
        s.excludedMove  = make_none();
        s.ply           = j - 7;
        s.staticEval    = VALUE_NONE;
        s.statScore     = 0;
        s.moveCount     = 0;
        s.cutoffCnt     = 0;
        s.reduction     = 0;
        s.pliesFromNull = 1000;
        s.capturedPt    = PT_NONE;
        s.inCheck       = false;
        s.ttPv          = false;
        s.ttHit         = false;
    }
}

inline void init_root_moves(ThreadWorker& w, const chess::Board& board) {
    chess::Movelist ml;
    chess::movegen::legalmoves(ml, board);
    w.root_moves.clear();
    w.root_moves.reserve(static_cast<std::size_t>(ml.size()));
    for (const Move& m : ml) {
        RootMove rm;
        rm.move = m;
        w.root_moves.push_back(rm);
    }
    w.root_initialized = true;
}

// Nueva iteracion en el hilo principal: decide si hay tiempo para empezar otra (logica de SF)
inline void iteration_start(ThreadWorker& w, int depth) {
    const int prevDepth = w.cur_iter_depth;
    w.cur_iter_depth    = depth;
    for (auto& rm : w.root_moves) rm.previousScore = rm.score;
    if (prevDepth <= 0 || w.id != 0) return;

    RootMove& best = w.root_moves[0];
    w.totBestMoveChanges += w.bestMoveChanges;
    w.bestMoveChanges = 0;
    if (best.move.move() != w.lastBestMove) {
        w.lastBestMove      = best.move.move();
        w.lastBestMoveDepth = prevDepth;
    }
    const Value bestValue = best.score;
    if (w.bestPreviousAverageScore == VALUE_INFINITE) {
        w.bestPreviousAverageScore = best.averageScore != -VALUE_INFINITE ? best.averageScore : bestValue;
        for (int& v : w.iterValue) v = bestValue;
    }

    const long long lim = g_time.hard_limit_ms.load(std::memory_order_relaxed);
    if (lim > 0 && !g_time.ponder_active.load(std::memory_order_relaxed) && prevDepth >= 2) {
        const double optimum = static_cast<double>(lim) * TM_OPTIMUM_FRACTION;
        double fallingEval = (11.396 + 2.035 * (w.bestPreviousAverageScore - bestValue) +
                              0.968 * (w.iterValue[w.iterIdx] - bestValue)) / 100.0;
        fallingEval = std::clamp(fallingEval, 0.5786, 1.6752);

        const double timeReduction = w.lastBestMoveDepth + 8 < prevDepth ? 1.4857 : 0.7046;
        const double reductionF    = (1.4540 + w.previousTimeReduction) / (2.1593 * timeReduction);
        const double instability   = 1 + 1.7 * w.totBestMoveChanges;
        const double nodes         = std::max<double>(1.0, static_cast<double>(static_cast<std::uint64_t>(w.nodos_busqueda)));
        const double effort        = static_cast<double>(best.effort) * 100000.0 / nodes;
        const double highEffort    = (prevDepth >= 10 && effort >= 93340) ? 0.76 : 1.0;

        double totalTime = optimum * fallingEval * reductionF * instability * highEffort;
        if (w.root_moves.size() == 1) totalTime = std::min(500.0, totalTime);

        w.iterValue[w.iterIdx] = bestValue;
        w.iterIdx = (w.iterIdx + 1) & 3;
        w.previousTimeReduction = timeReduction;

        w.totBestMoveChanges /= 2;
        if (static_cast<double>(limit_elapsed_ms()) > std::min(totalTime, static_cast<double>(lim))) {
            stop_search.store(true, std::memory_order_relaxed);
            throw SearchAbortedException();
        }
        return;
    }
    w.iterValue[w.iterIdx] = bestValue;
    w.iterIdx = (w.iterIdx + 1) & 3;
    w.totBestMoveChanges /= 2;
}

}  // namespace fb

// negamax(): punto de entrada usado por SearchController.cpp
//   ply == 0  -> nodo raiz (lista de jugadas raiz, orden entre iteraciones, tiempo)
//   ply >= 1  -> subarbol PV (lo usa el MultiPV del controlador)
inline int negamax(ThreadWorker& w, chess::Board& board, int depth, int alpha, int beta, int ply) {
    using namespace fb;

    if (!w.data) w.data = worker_data(w.id);
    w.tt  = &tt_instance();
    w.ctx = &rubi_context();
    (void)tables();

    ply = std::clamp(ply, 0, 8);
    init_worker_stack(w);
    Stack* ss = &w.stack[7 + ply];
    w.root_stm = (ply & 1) ? ~board.sideToMove() : board.sideToMove();
    ss->pliesFromNull = 1000;

    const std::size_t need = w.pos_history.size() + MAX_PLY + 16;
    if (w.pos_history.capacity() < need) w.pos_history.reserve(need);
    if (w.pos_history.empty() || w.pos_history.back() != board.hash()) w.pos_history.push_back(board.hash());

    if (ply == 0) {
        if (!w.root_initialized) init_root_moves(w, board);
        if (w.root_moves.empty()) return board.inCheck() ? mated_in(0) : VALUE_DRAW;

        if (depth != w.cur_iter_depth) iteration_start(w, depth);
        w.rootDepth = depth;

        const Value v = search<Root>(w, board, ss, alpha, beta, depth, false);

        // Ordenar jugadas raiz: score desc, desempate por score previo (sort estable, como SF)
        std::stable_sort(w.root_moves.begin(), w.root_moves.end(), [](const RootMove& a, const RootMove& b) {
            return a.score != b.score ? a.score > b.score : a.previousScore > b.previousScore;
        });
        w.completedDepth = depth;
        return v;
    }

    w.rootDepth = depth + ply;
    w.pv_length[ply] = 0;
    return search<PV>(w, board, ss, alpha, beta, depth, false);
}