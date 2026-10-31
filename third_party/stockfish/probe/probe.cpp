#include <memory>

#include "probe.h"
#include "bitboard.h"
#include "position.h"
#include "evaluate.h"
#include "nnue/network.h"
#include "nnue/nnue_accumulator.h"

namespace Stockfish::Probe {

std::unique_ptr<Eval::NNUE::Networks> networks;

namespace {

struct ProbeTLS {
    std::unique_ptr<Eval::NNUE::AccumulatorStack> accumulators;
    std::unique_ptr<Eval::NNUE::AccumulatorCaches> caches;
};

// TLS mediante puntero para evitar problemas de inicialización TLS con LTO.
inline thread_local ProbeTLS* tls = nullptr;

[[gnu::always_inline]]
inline ProbeTLS& context() noexcept
{
    if (tls == nullptr) [[unlikely]]
    {
        tls = new ProbeTLS;

        tls->accumulators =
            std::make_unique<Eval::NNUE::AccumulatorStack>();

        tls->caches =
            std::make_unique<Eval::NNUE::AccumulatorCaches>(*networks);
    }

    return *tls;
}

[[gnu::always_inline]]
inline int evaluate_position(
    Position& pos,
    StateInfo& state,
    int optimism,
    Evaluation* details = nullptr) noexcept
{
    auto& ctx = context();

    ctx.accumulators->reset();

    Eval::EvalResult result;
    const int value = Eval::evaluate(
        *networks,
        pos,
        *ctx.accumulators,
        *ctx.caches,
        optimism,
        details ? &result : nullptr
    );

    if (details)
        *details = {result.psqt, result.positional, result.nnue, value, result.smallNet};

    return value;
}

[[gnu::always_inline]]
inline RawEval evaluate_raw_position(
    Position& pos,
    StateInfo& state) noexcept
{
    auto& ctx = context();

    ctx.accumulators->reset();

    bool smallNet = Eval::use_smallnet(pos);

    Eval::NNUE::NetworkOutput output =
        smallNet
            ? networks->small.evaluate(
                  pos,
                  *ctx.accumulators,
                  ctx.caches->small
              )
            : networks->big.evaluate(
                  pos,
                  *ctx.accumulators,
                  ctx.caches->big
              );

    int psqt = static_cast<int>(std::get<0>(output));
    int positional = static_cast<int>(std::get<1>(output));

    int nnue =
        (125 * psqt + 131 * positional) / 128;

    // Igual que Stockfish: si la small net queda cerca de 0,
    // vuelve a evaluar con la big net.
    if (smallNet && std::abs(nnue) < 236)
    {
        output =
            networks->big.evaluate(
                pos,
                *ctx.accumulators,
                ctx.caches->big
            );

        psqt = static_cast<int>(std::get<0>(output));
        positional = static_cast<int>(std::get<1>(output));

        nnue =
            (125 * psqt + 131 * positional) / 128;
    }

    return { psqt, positional, nnue };
}

} // namespace

void init(const char* bigNetFile, const char* smallNetFile)
{
    Bitboards::init();
    Position::init();
    Eval::NNUE::Features::init_threat_offsets();

    networks = std::make_unique<Eval::NNUE::Networks>(
        std::make_unique<Eval::NNUE::NetworkBig>(
            Eval::NNUE::EvalFile{
                bigNetFile,
                "None",
                ""
            },
            Eval::NNUE::EmbeddedNNUEType::BIG
        ),

        std::make_unique<Eval::NNUE::NetworkSmall>(
            Eval::NNUE::EvalFile{
                smallNetFile,
                "None",
                ""
            },
            Eval::NNUE::EmbeddedNNUEType::SMALL
        )
    );

    networks->big.load("", bigNetFile);
    networks->small.load("", smallNetFile);
}


int eval(const char* fen, int optimism)
{
    Position pos;
    StateInfo state;

    pos.set(fen, &state);

    return evaluate_position(pos, state, optimism);
}

Evaluation eval_detailed(const char* fen, int optimism)
{
    Position pos;
    StateInfo state;
    Evaluation result{};

    pos.set(fen, &state);
    evaluate_position(pos, state, optimism, &result);
    return result;
}

RawEval raw_eval(const char* fen)
{
    Position pos;
    StateInfo state;

    pos.set(fen, &state);

    return evaluate_raw_position(pos, state);
}


int eval(const int pieceBoard[], bool side, int rule50, int optimism)
{
    Position pos;
    StateInfo state;

    pos.set(
        pieceBoard,
        side,
        rule50,
        &state
    );

    return evaluate_position(pos, state, optimism);
}

Evaluation eval_detailed(const int pieceBoard[], bool side, int rule50, int optimism)
{
    Position pos;
    StateInfo state;
    Evaluation result{};

    pos.set(pieceBoard, side, rule50, &state);
    evaluate_position(pos, state, optimism, &result);
    return result;
}

RawEval raw_eval(
    const int pieceBoard[],
    bool side,
    int rule50)
{
    Position pos;
    StateInfo state;

    pos.set(
        pieceBoard,
        side,
        rule50,
        &state
    );

    return evaluate_raw_position(pos, state);
}


int eval(
    const int pieces[],
    const int squares[],
    int pieceAmount,
    bool side,
    int rule50,
    int optimism)
{
    Position pos;
    StateInfo state;

    pos.set(
        pieces,
        squares,
        pieceAmount,
        side,
        rule50,
        &state
    );

    return evaluate_position(pos, state, optimism);
}

Evaluation eval_detailed(
    const int pieces[],
    const int squares[],
    int pieceAmount,
    bool side,
    int rule50,
    int optimism)
{
    Position pos;
    StateInfo state;
    Evaluation result{};

    pos.set(pieces, squares, pieceAmount, side, rule50, &state);
    evaluate_position(pos, state, optimism, &result);
    return result;
}

RawEval raw_eval(
    const int pieces[],
    const int squares[],
    int pieceAmount,
    bool side,
    int rule50)
{
    Position pos;
    StateInfo state;

    pos.set(
        pieces,
        squares,
        pieceAmount,
        side,
        rule50,
        &state
    );

    return evaluate_raw_position(pos, state);
}

} // namespace Stockfish::Probe