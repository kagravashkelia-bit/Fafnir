
"""
100-position chess-engine regression suite.

The positions are public EPD test positions collected from established suites:
  - Bratko-Kopec (24)
  - Kaufman (19)
  - Nolot (11)
  - LCT-II (14 positional + 12 combination)
  - Rybka Forum (20)

The test is intentionally strict: a position is a HIT only when the engine's
UCI bestmove matches one of the EPD 'bm' moves.  The runner converts the EPD
SAN best moves to UCI internally, so the test file stays readable.

Requires Python 3.9+ only. No third-party packages are required.

Usage examples (Windows):
    python tests.py nanobit.exe --depth 18
    python tests.py nanobit.exe --movetime 1000
    python tests.py nanobit.exe --depth 20 --hash 128 --threads 1
    python tests.py nanobit.exe --depth 18 --category FINALES_RYBKA
"""

from __future__ import annotations

import argparse
import os
import queue
import re
import shutil
import subprocess
import sys
import threading
import time
from dataclasses import dataclass
from typing import Dict, Iterable, List, Optional, Sequence, Tuple


# ============================================================================
# TEST DATA
# ============================================================================

@dataclass(frozen=True)
class TestCase:
    name: str
    category: str
    fen: str
    best_moves: Tuple[str, ...]
    source: str


# Historical EPD positions.  We preserve the original FEN board/side/castling/
# EP fields; EPD files often omit halfmove/fullmove counters, so the runner adds
# neutral counters when sending a FEN to the engine.
TESTS: List[TestCase] = [
    # ----------------------------------------------------------------------
    # Bratko-Kopec: tactical calculation / positional tactics (24)
    # ----------------------------------------------------------------------
    TestCase("BK.01", "TACTICA_BK", "1k1r4/pp1b1R2/3q2pp/4p3/2B5/4Q3/PPP2B2/2K5 b - -", ("Qd1",), "Bratko-Kopec"),
    TestCase("BK.02", "TACTICA_BK", "3r1k2/4npp1/1ppr3p/p6P/P2PPPP1/1NR5/5K2/2R5 w - -", ("d5",), "Bratko-Kopec"),
    TestCase("BK.03", "TACTICA_BK", "2q1rr1k/3bbnnp/p2p1pp1/2pPp3/PpP1P1P1/1P2BNNP/2BQ1PRK/7R b - -", ("f5",), "Bratko-Kopec"),
    TestCase("BK.04", "TACTICA_BK", "rnbqkb1r/p3pppp/1p6/2ppP3/3N4/2P5/PPP1QPPP/R1B1KB1R w KQkq -", ("e6",), "Bratko-Kopec"),
    TestCase("BK.05", "TACTICA_BK", "r1b2rk1/2q1b1pp/p2ppn2/1p6/3QP3/1BN1B3/PPP3PP/R4RK1 w - -", ("Nd5", "a4"), "Bratko-Kopec"),
    TestCase("BK.06", "TACTICA_BK", "2r3k1/pppR1pp1/4p3/4P1P1/5P2/1P4K1/P1P5/8 w - -", ("g6",), "Bratko-Kopec"),
    TestCase("BK.07", "TACTICA_BK", "1nk1r1r1/pp2n1pp/4p3/q2pPp1N/b1pP1P2/B1P2R2/2P1B1PP/R2Q2K1 w - -", ("Nf6",), "Bratko-Kopec"),
    TestCase("BK.08", "TACTICA_BK", "4b3/p3kp2/6p1/3pP2p/2pP1P2/4K1P1/P3N2P/8 w - -", ("f5",), "Bratko-Kopec"),
    TestCase("BK.09", "TACTICA_BK", "2kr1bnr/pbpq4/2n1pp2/3p3p/3P1P1B/2N2N1Q/PPP3PP/2KR1B1R w - -", ("f5",), "Bratko-Kopec"),
    TestCase("BK.10", "TACTICA_BK", "3rr1k1/pp3pp1/1qn2np1/8/3p4/PP1R1P2/2P1NQPP/R1B3K1 b - -", ("Ne5",), "Bratko-Kopec"),
    TestCase("BK.11", "TACTICA_BK", "2r1nrk1/p2q1ppp/bp1p4/n1pPp3/P1P1P3/2PBB1N1/4QPPP/R4RK1 w - -", ("f4",), "Bratko-Kopec"),
    TestCase("BK.12", "TACTICA_BK", "r3r1k1/ppqb1ppp/8/4p1NQ/8/2P5/PP3PPP/R3R1K1 b - -", ("Bf5",), "Bratko-Kopec"),
    TestCase("BK.13", "TACTICA_BK", "r2q1rk1/4bppp/p2p4/2pP4/3pP3/3Q4/PP1B1PPP/R3R1K1 w - -", ("b4",), "Bratko-Kopec"),
    TestCase("BK.14", "TACTICA_BK", "rnb2r1k/pp2p2p/2pp2p1/q2P1p2/8/1Pb2NP1/PB2PPBP/R2Q1RK1 w - -", ("Qd2", "Qe1"), "Bratko-Kopec"),
    TestCase("BK.15", "TACTICA_BK", "2r3k1/1p2q1pp/2b1pr2/p1pp4/6Q1/1P1PP1R1/P1PN2PP/5RK1 w - -", ("Qxg7+",), "Bratko-Kopec"),
    TestCase("BK.16", "TACTICA_BK", "r1bqkb1r/4npp1/p1p4p/1p1pP1B1/8/1B6/PPPN1PPP/R2Q1RK1 w kq -", ("Ne4",), "Bratko-Kopec"),
    TestCase("BK.17", "TACTICA_BK", "r2q1rk1/1ppnbppp/p2p1nb1/3Pp3/2P1P1P1/2N2N1P/PPB1QP2/R1B2RK1 b - -", ("h5",), "Bratko-Kopec"),
    TestCase("BK.18", "TACTICA_BK", "r1bq1rk1/pp2ppbp/2np2p1/2n5/P3PP2/N1P2N2/1PB3PP/R1B1QRK1 b - -", ("Nb3",), "Bratko-Kopec"),
    TestCase("BK.19", "TACTICA_BK", "3rr3/2pq2pk/p2p1pnp/8/2QBPP2/1P6/P5PP/4RRK1 b - -", ("Rxe4",), "Bratko-Kopec"),
    TestCase("BK.20", "TACTICA_BK", "r4k2/pb2bp1r/1p1qp2p/3pNp2/3P1P2/2N3P1/PPP1Q2P/2KRR3 w - -", ("g4",), "Bratko-Kopec"),
    TestCase("BK.21", "TACTICA_BK", "3rn2k/ppb2rpp/2ppqp2/5N2/2P1P3/1P5Q/PB3PPP/3RR1K1 w - -", ("Nh6",), "Bratko-Kopec"),
    TestCase("BK.22", "TACTICA_BK", "2r2rk1/1bqnbpp1/1p1ppn1p/pP6/N1P1P3/P2B1N1P/1B2QPP1/R2R2K1 b - -", ("Bxe4",), "Bratko-Kopec"),
    TestCase("BK.23", "TACTICA_BK", "r1bqk2r/pp2bppp/2p5/3pP3/P2Q1P2/2N1B3/1PP3PP/R4RK1 b kq -", ("f6",), "Bratko-Kopec"),
    TestCase("BK.24", "TACTICA_BK", "r2qnrnk/p2b2b1/1p1p2pp/2pPpp2/1PP1P3/PRNBB3/3QNPPP/5RK1 w - -", ("f4",), "Bratko-Kopec"),

    # ----------------------------------------------------------------------
    # Kaufman: opening/strategic accuracy (19 bm positions; 'am' positions
    # deliberately excluded because this first runner is strict best-move only)
    # ----------------------------------------------------------------------
    TestCase("K.01", "ESTRATEGIA_KAUFMAN", "rn1qkb1r/pp2pppp/5n2/3p1b2/3P4/2N1P3/PP3PPP/R1BQKBNR w KQkq -", ("Qb3",), "Kaufman"),
    TestCase("K.02", "ESTRATEGIA_KAUFMAN", "rn1qkb1r/pp2pppp/5n2/3p1b2/3P4/1QN1P3/PP3PPP/R1B1KBNR b KQkq -", ("Bc8",), "Kaufman"),
    TestCase("K.03", "ESTRATEGIA_KAUFMAN", "r1bqk2r/ppp2ppp/2n5/4P3/2Bp2n1/5N1P/PP1N1PP1/R2Q1RK1 b kq -", ("Nh6",), "Kaufman"),
    TestCase("K.04", "ESTRATEGIA_KAUFMAN", "r1bqrnk1/pp2bp1p/2p2np1/3p2B1/3P4/2NBPN2/PPQ2PPP/1R3RK1 w - -", ("b4",), "Kaufman"),
    TestCase("K.05", "ESTRATEGIA_KAUFMAN", "rnbqkb1r/ppp1pppp/5n2/8/3PP3/2N5/PP3PPP/R1BQKBNR b KQkq -", ("e5",), "Kaufman"),
    # Source line has the historical typo "Bcx3+"; the intended SAN is Bxc3+.
    TestCase("K.06", "ESTRATEGIA_KAUFMAN", "rnbq1rk1/pppp1ppp/4pn2/8/1bPP4/P1N5/1PQ1PPPP/R1B1KBNR b KQ -", ("Bxc3+",), "Kaufman"),
    TestCase("K.07", "ESTRATEGIA_KAUFMAN", "r4rk1/3nppbp/bq1p1np1/2pP4/8/2N2NPP/PP2PPB1/R1BQR1K1 b - -", ("Rfb8",), "Kaufman"),
    TestCase("K.08", "ESTRATEGIA_KAUFMAN", "rn1qkb1r/pb1p1ppp/1p2pn2/2p5/2PP4/5NP1/PP2PPBP/RNBQK2R w KQkq c6", ("d5",), "Kaufman"),
    TestCase("K.09", "ESTRATEGIA_KAUFMAN", "r1bq1rk1/1pp2pbp/p1np1np1/3Pp3/2P1P3/2N1BP2/PP4PP/R1NQKB1R b KQ -", ("Nd4",), "Kaufman"),
    TestCase("K.10", "ESTRATEGIA_KAUFMAN", "rnbqr1k1/1p3pbp/p2p1np1/2pP4/4P3/2N5/PP1NBPPP/R1BQ1RK1 w - -", ("a4",), "Kaufman"),
    TestCase("K.11", "ESTRATEGIA_KAUFMAN", "rnbqkb1r/pppp1ppp/5n2/4p3/4PP2/2N5/PPPP2PP/R1BQKBNR b KQkq f3", ("d5",), "Kaufman"),
    TestCase("K.12", "ESTRATEGIA_KAUFMAN", "r1bqk1nr/pppnbppp/3p4/8/2BNP3/8/PPP2PPP/RNBQK2R w KQkq -", ("Bxf7+",), "Kaufman"),
    TestCase("K.13", "ESTRATEGIA_KAUFMAN", "r2q1rk1/2p1bppp/p2p1n2/1p2P3/4P1b1/1nP1BN2/PP3PPP/RN1QR1K1 w - -", ("exf6",), "Kaufman"),
    TestCase("K.14", "ESTRATEGIA_KAUFMAN", "r1bqkb1r/2pp1ppp/p1n5/1p2p3/3Pn3/1B3N2/PPP2PPP/RNBQ1RK1 b kq -", ("d5",), "Kaufman"),
    TestCase("K.15", "ESTRATEGIA_KAUFMAN", "r1bqkb1r/pp3ppp/2np1n2/4p1B1/3NP3/2N5/PPP2PPP/R2QKB1R w KQkq e6", ("Bxf6+",), "Kaufman"),
    TestCase("K.16", "ESTRATEGIA_KAUFMAN", "r3kb1r/pp1n1ppp/1q2p3/n2p4/3P1Bb1/2PB1N2/PPQ2PPP/RN2K2R w KQkq -", ("a4",), "Kaufman"),
    TestCase("K.17", "ESTRATEGIA_KAUFMAN", "r1bq1rk1/pppnnppp/4p3/3pP3/1b1P4/2NB3N/PPP2PPP/R1BQK2R w KQ -", ("Bxh7+",), "Kaufman"),
    TestCase("K.18", "ESTRATEGIA_KAUFMAN", "r2qkbnr/ppp1pp1p/3p2p1/3Pn3/4P1b1/2N2N2/PPP2PPP/R1BQKB1R w KQkq -", ("Nxe5",), "Kaufman"),
    # Kaufman has 18 strict bm entries in this public file; the 19th slot is
    # filled with a distinct LCT-II final rather than duplicating a position.
    TestCase("LCT.FIN.01", "FINALES_LCT2", "8/1p3pp1/7p/5P1P/2k3P1/8/2K2P2/8 w - -", ("f6",), "LCT-II"),

    # ----------------------------------------------------------------------
    # Nolot: sharp tactical positions (11)
    # ----------------------------------------------------------------------
    TestCase("N.01", "CALCULO_NOLOT", "r3qb1k/1b4p1/p2pr2p/3n4/Pnp1N1N1/6RP/1B3PP1/1B1QR1K1 w - -", ("Nxh6",), "Nolot"),
    TestCase("N.02", "CALCULO_NOLOT", "r4rk1/pp1n1p1p/1nqP2p1/2b1P1B1/4NQ2/1B3P2/PP2K2P/2R5 w - -", ("Rxc5",), "Nolot"),
    TestCase("N.03", "CALCULO_NOLOT", "r2qk2r/ppp1b1pp/2n1p3/3pP1n1/3P2b1/2PB1NN1/PP4PP/R1BQK2R w KQkq -", ("Nxg5",), "Nolot"),
    TestCase("N.04", "CALCULO_NOLOT", "r1b1kb1r/1p1n1ppp/p2ppn2/6BB/2qNP3/2N5/PPP2PPP/R2Q1RK1 w kq -", ("Nxe6",), "Nolot"),
    TestCase("N.05", "CALCULO_NOLOT", "r2qrb1k/1p1b2p1/p2ppn1p/8/3NP3/1BN5/PPP3QP/1K3RR1 w - -", ("e5",), "Nolot"),
    TestCase("N.06", "CALCULO_NOLOT", "rnbqk2r/1p3ppp/p7/1NpPp3/QPP1P1n1/P4N2/4KbPP/R1B2B1R b kq -", ("axb5",), "Nolot"),
    TestCase("N.07", "CALCULO_NOLOT", "1r1bk2r/2R2ppp/p3p3/1b2P2q/4QP2/4N3/1B4PP/3R2K1 w k -", ("Rxd8+",), "Nolot"),
    TestCase("N.08", "CALCULO_NOLOT", "r3rbk1/ppq2ppp/2b1pB2/8/6Q1/1P1B3P/P1P2PP1/R2R2K1 w - -", ("Bxh7+",), "Nolot"),
    TestCase("N.09", "CALCULO_NOLOT", "r4r1k/4bppb/2n1p2p/p1n1P3/1p1p1BNP/3P1NP1/qP2QPB1/2RR2K1 w - -", ("Ng5",), "Nolot"),
    TestCase("N.10", "CALCULO_NOLOT", "r1b2rk1/1p1nbppp/pq1p4/3B4/P2NP3/2N1p3/1PP3PP/R2Q1R1K w - -", ("Rxf7",), "Nolot"),
    TestCase("N.11", "CALCULO_NOLOT", "r1b3k1/p2p1nP1/2pqr1Rp/1p2p2P/2B1PnQ1/1P6/P1PP4/1K4R1 w - -", ("Rxh6",), "Nolot"),

    # ----------------------------------------------------------------------
    # LCT-II positional (14)
    # ----------------------------------------------------------------------
    TestCase("LCT.POS.01", "POSICIONAL_LCT2", "r3kb1r/3n1pp1/p6p/2pPp2q/Pp2N3/3B2PP/1PQ2P2/R3K2R w KQkq -", ("d6",), "LCT-II"),
    TestCase("LCT.POS.02", "POSICIONAL_LCT2", "1k1r3r/pp2qpp1/3b1n1p/3pNQ2/2pP1P2/2N1P3/PP4PP/1K1RR3 b - -", ("Bb4",), "LCT-II"),
    TestCase("LCT.POS.03", "POSICIONAL_LCT2", "r6k/pp4p1/2p1b3/3pP3/7q/P2B3r/1PP2Q1P/2K1R1R1 w - -", ("Qc5",), "LCT-II"),
    TestCase("LCT.POS.04", "POSICIONAL_LCT2", "1nr5/2rbkppp/p3p3/Np6/2PRPP2/8/PKP1B1PP/3R4 b - -", ("e5",), "LCT-II"),
    TestCase("LCT.POS.05", "POSICIONAL_LCT2", "2r2rk1/1p1bq3/p3p2p/3pPpp1/1P1Q4/P7/2P2PPP/2R1RBK1 b - -", ("Bb5",), "LCT-II"),
    TestCase("LCT.POS.06", "POSICIONAL_LCT2", "3r1bk1/p4ppp/Qp2p3/8/1P1B4/Pq2P1P1/2r2P1P/R3R1K1 b - -", ("e5",), "LCT-II"),
    TestCase("LCT.POS.07", "POSICIONAL_LCT2", "r1b2r1k/pp2q1pp/2p2p2/2p1n2N/4P3/1PNP2QP/1PP2RP1/5RK1 w - -", ("Nd1",), "LCT-II"),
    TestCase("LCT.POS.08", "POSICIONAL_LCT2", "r2qrnk1/pp3ppb/3b1n1p/1Pp1p3/2P1P2N/P5P1/1B1NQPBP/R4RK1 w - -", ("Bh3",), "LCT-II"),
    TestCase("LCT.POS.09", "POSICIONAL_LCT2", "5nk1/Q4bpp/5p2/8/P1n1PN2/q4P2/6PP/1R4K1 w - -", ("Qd4",), "LCT-II"),
    TestCase("LCT.POS.10", "POSICIONAL_LCT2", "r3k2r/3bbp1p/p1nppp2/5P2/1p1NP3/5NP1/PPPK3P/3R1B1R b kq -", ("Bf8",), "LCT-II"),
    TestCase("LCT.POS.11", "POSICIONAL_LCT2", "bn6/1q4n1/1p1p1kp1/2pPp1pp/1PP1P1P1/3N1P1P/4B1K1/2Q2N2 w - -", ("h4",), "LCT-II"),
    TestCase("LCT.POS.12", "POSICIONAL_LCT2", "3r2k1/pp2npp1/2rqp2p/8/3PQ3/1BR3P1/PP3P1P/3R2K1 b - -", ("Rb6",), "LCT-II"),
    TestCase("LCT.POS.13", "POSICIONAL_LCT2", "1r2r1k1/4ppbp/B5p1/3P4/pp1qPB2/2n2Q1P/P4PP1/4RRK1 b - -", ("Nxa2",), "LCT-II"),
    TestCase("LCT.POS.14", "POSICIONAL_LCT2", "r2qkb1r/1b3ppp/p3pn2/1p6/1n1P4/1BN2N2/PP2QPPP/R1BR2K1 w kq -", ("d5",), "LCT-II"),

    # ----------------------------------------------------------------------
    # LCT-II combinations (12)
    # ----------------------------------------------------------------------
    TestCase("LCT.CMB.01", "COMBINACIONES_LCT2", "1r4k1/1q2bp2/3p2p1/2pP4/p1N4R/2P2QP1/1P3PK1/8 w - -", ("Nxd6",), "LCT-II"),
    TestCase("LCT.CMB.02", "COMBINACIONES_LCT2", "rn3rk1/pbppq1pp/1p2pb2/4N2Q/3PN3/3B4/PPP2PPP/R3K2R w KQ -", ("Qxh7+",), "LCT-II"),
    TestCase("LCT.CMB.03", "COMBINACIONES_LCT2", "4r1k1/3b1p2/5qp1/1BPpn2p/7n/r3P1N1/2Q1RPPP/1R3NK1 b - -", ("Qf3",), "LCT-II"),
    TestCase("LCT.CMB.04", "COMBINACIONES_LCT2", "2k2b1r/1pq3p1/2p1pp2/p1n1PnNp/2P2B2/2N4P/PP2QPP1/3R2K1 w - -", ("exf6",), "LCT-II"),
    TestCase("LCT.CMB.05", "COMBINACIONES_LCT2", "2r2r2/3qbpkp/p3n1p1/2ppP3/6Q1/1P1B3R/PBP3PP/5R1K w - -", ("Rxh7+",), "LCT-II"),
    TestCase("LCT.CMB.06", "COMBINACIONES_LCT2", "2r1k2r/2pn1pp1/1p3n1p/p3PP2/4q2B/P1P5/2Q1N1PP/R4RK1 w q -", ("exf6",), "LCT-II"),
    TestCase("LCT.CMB.07", "COMBINACIONES_LCT2", "2rr2k1/1b3ppp/pb2p3/1p2P3/1P2BPnq/P1N3P1/1B2Q2P/R4R1K b - -", ("Rxc3",), "LCT-II"),
    TestCase("LCT.CMB.08", "COMBINACIONES_LCT2", "2b1r1k1/r4ppp/p7/2pNP3/4Q3/q6P/2P2PP1/3RR1K1 w - -", ("Nf6+",), "LCT-II"),
    TestCase("LCT.CMB.09", "COMBINACIONES_LCT2", "6k1/5p2/3P2p1/7n/3QPP2/7q/r2N3P/6RK b - -", ("Rxd2",), "LCT-II"),
    TestCase("LCT.CMB.10", "COMBINACIONES_LCT2", "rq2rbk1/6p1/p2p2Pp/1p1Rn3/4PB2/6Q1/PPP1B3/2K3R1 w - -", ("Bxh6",), "LCT-II"),
    TestCase("LCT.CMB.11", "COMBINACIONES_LCT2", "rnbq2k1/p1r2p1p/1p1p1Pp1/1BpPn1N1/P7/2P5/6PP/R1B1QRK1 w - -", ("Nxh7",), "LCT-II"),
    TestCase("LCT.FIN.02", "FINALES_LCT2", "8/pp2r1k1/2p1p3/3pP2p/1P1P1P1P/P5KR/8/8 w - -", ("f5",), "LCT-II"),

    # ----------------------------------------------------------------------
    # Rybka Forum: mixed / many precision endgames (20 selected positions)
    # ----------------------------------------------------------------------
    TestCase("RF.01", "FINALES_RYBKA", "6N1/7b/8/7p/p3k3/6KN/8/8 w - -", ("Ng5+",), "Rybka Forum"),
    TestCase("RF.02", "FINALES_RYBKA", "8/8/8/8/2RK3R/6k1/1Pb2pp1/8 w - -", ("Ke3",), "Rybka Forum"),
    TestCase("RF.03", "FINALES_RYBKA", "4rk2/7p/p1nq2p1/7r/Q1P2p2/2B2P2/P3P2P/1N2R2K w - -", ("c5",), "Rybka Forum"),
    TestCase("RF.04", "FINALES_RYBKA", "6rk/p1r2qp1/1n3p1p/3pp3/p7/B1PP1Q2/B1P4R/6RK b - -", ("e4",), "Rybka Forum"),
    TestCase("RF.05", "FINALES_RYBKA", "2Q5/8/8/8/3kbP1N/2pq3P/6PK/8 w - -", ("f5",), "Rybka Forum"),
    TestCase("RF.06", "FINALES_RYBKA", "6k1/p4p1p/4pbp1/1p1r4/q1rP1P2/2P2R1P/PB4P1/R2Q3K w - -", ("Qxa4",), "Rybka Forum"),
    TestCase("RF.07", "FINALES_RYBKA", "3q1r1k/4n3/1R3p1p/3p1b2/3P4/Q3RBPp/4PP1K/2r2N2 b - -", ("Rxf1",), "Rybka Forum"),
    TestCase("RF.08", "FINALES_RYBKA", "3B4/1r2p3/r2p1p2/bkp1P1p1/1p1P1PPp/p1P1K2P/PPB5/8 w - -", ("Ba4+",), "Rybka Forum"),
    TestCase("RF.09", "FINALES_RYBKA", "q2r2k1/1bpn1ppp/1p2p3/4P1N1/r2P4/b5P1/1BQ2PBP/3RR1K1 b - -", ("Nf8",), "Rybka Forum"),
    TestCase("RF.10", "FINALES_RYBKA", "5r2/1b2k2p/3bp3/5PP1/Bn1Q3P/1N6/1Pr5/1K3R2 b - -", ("Rxb2+",), "Rybka Forum"),
    TestCase("RF.11", "FINALES_RYBKA", "8/5p2/8/4k3/4p2R/2pp2P1/2p5/4KB1R w K -", ("Rxe4+",), "Rybka Forum"),
    TestCase("RF.12", "FINALES_RYBKA", "5k2/7R/8/4Kp2/5Pp1/P5rp/1P6/8 w - -", ("Kf6",), "Rybka Forum"),
    TestCase("RF.13", "FINALES_RYBKA", "r4rk1/pbp2ppp/1p3Q2/8/2P5/2BR3P/PP2qPP1/5RK1 b - -", ("Qg4",), "Rybka Forum"),
    TestCase("RF.14", "FINALES_RYBKA", "8/3k4/1p4b1/3p1p1p/K2P1PpP/3B2P1/1P6/8 w - -", ("Kb5",), "Rybka Forum"),
    TestCase("RF.15", "FINALES_RYBKA", "8/2B2k2/6p1/5P1p/6p1/p1p3PP/6P1/1n2K2R w K -", ("fxg6+",), "Rybka Forum"),
    TestCase("RF.16", "FINALES_RYBKA", "8/3r2p1/pR5p/5P1K/P4kP1/8/8/8 b - -", ("a5",), "Rybka Forum"),
    TestCase("RF.17", "FINALES_RYBKA", "8/8/8/2k5/p1b5/8/1P1K4/8 w - -", ("Kc2",), "Rybka Forum"),
    TestCase("RF.18", "FINALES_RYBKA", "r1b1k3/1p1p1pp1/1p1P3p/pP6/P7/8/5PPP/4R1K1 b q -", ("Kf8",), "Rybka Forum"),
    TestCase("RF.19", "FINALES_RYBKA", "5k2/1p2pPp1/3pP1P1/2p1pBp1/2K1P1Pp/8/8/N7 w - -", ("Nb3",), "Rybka Forum"),
    TestCase("RF.20", "FINALES_RYBKA", "7k/6np/3B3N/6P1/8/8/K7/5q2 w - -", ("Kb2",), "Rybka Forum"),
]


# ============================================================================
# MINIMAL CHESS/SAN PARSER (no python-chess dependency)
# ============================================================================

FILES = "abcdefgh"
RANKS = "12345678"
PIECE_TYPES = "KQRBN"


def square_index(sq: str) -> Tuple[int, int]:
    return FILES.index(sq[0]), int(sq[1]) - 1


def index_square(x: int, y: int) -> str:
    return FILES[x] + str(y + 1)


def normalize_position_fen(fen: str) -> str:
    p = fen.split()
    if len(p) < 4:
        raise ValueError(f"Bad FEN/EPD core: {fen}")
    return " ".join([p[0], p[1], p[2], p[3], "0", "1"])


def board_from_fen(fen: str) -> Tuple[Dict[str, str], str, str]:
    p = fen.split()
    rows = p[0].split("/")
    if len(rows) != 8:
        raise ValueError("Bad board in FEN")
    board: Dict[str, str] = {}
    for row_i, row in enumerate(rows):
        y = 7 - row_i
        x = 0
        for ch in row:
            if ch.isdigit():
                x += int(ch)
            else:
                if x >= 8:
                    raise ValueError("Bad FEN rank")
                board[index_square(x, y)] = ch
                x += 1
        if x != 8:
            raise ValueError("Bad FEN rank width")
    side = p[1]
    ep = p[3]
    return board, side, ep


def piece_color(piece: str) -> str:
    return "w" if piece.isupper() else "b"


def own(piece: Optional[str], side: str) -> bool:
    return piece is not None and piece_color(piece) == side


def enemy(piece: Optional[str], side: str) -> bool:
    return piece is not None and piece_color(piece) != side


def ray_clear(board: Dict[str, str], src: str, dst: str) -> bool:
    sx, sy = square_index(src)
    dx, dy = square_index(dst)
    stepx = 0 if dx == sx else (1 if dx > sx else -1)
    stepy = 0 if dy == sy else (1 if dy > sy else -1)
    x, y = sx + stepx, sy + stepy
    while (x, y) != (dx, dy):
        if index_square(x, y) in board:
            return False
        x += stepx
        y += stepy
    return True


def pseudo_attacks(board: Dict[str, str], src: str, dst: str) -> bool:
    piece = board[src]
    p = piece.upper()
    sx, sy = square_index(src)
    dx, dy = square_index(dst)
    ax, ay = abs(dx - sx), abs(dy - sy)
    if p == "N":
        return (ax, ay) in ((1, 2), (2, 1))
    if p == "K":
        return max(ax, ay) == 1
    if p == "B":
        return ax == ay and ax > 0 and ray_clear(board, src, dst)
    if p == "R":
        return ((ax == 0) ^ (ay == 0)) and ray_clear(board, src, dst)
    if p == "Q":
        return ((ax == 0) ^ (ay == 0) or ax == ay) and (ax + ay > 0) and ray_clear(board, src, dst)
    if p == "P":
        direction = 1 if piece.isupper() else -1
        return dy - sy == direction and ax == 1
    return False


def king_square(board: Dict[str, str], side: str) -> Optional[str]:
    target = "K" if side == "w" else "k"
    for sq, p in board.items():
        if p == target:
            return sq
    return None


def attacked(board: Dict[str, str], target: str, by_side: str) -> bool:
    tx, ty = square_index(target)
    # pawns
    py = ty - (1 if by_side == "w" else -1)
    for px in (tx - 1, tx + 1):
        if 0 <= px < 8 and 0 <= py < 8:
            sq = index_square(px, py)
            if board.get(sq) == ("P" if by_side == "w" else "p"):
                return True
    # knights, kings, sliders
    for src, piece in board.items():
        if piece_color(piece) != by_side:
            continue
        p = piece.upper()
        sx, sy = square_index(src)
        ax, ay = abs(tx - sx), abs(ty - sy)
        if p == "N" and (ax, ay) in ((1, 2), (2, 1)):
            return True
        if p == "K" and max(ax, ay) == 1:
            return True
        if p in "BRQ" and pseudo_attacks(board, src, target):
            return True
    return False


def make_move(board: Dict[str, str], side: str, src: str, dst: str, ep: str) -> Dict[str, str]:
    b = dict(board)
    piece = b.pop(src)
    if piece.upper() == "P" and dst == ep and dst not in b:
        dx, dy = square_index(dst)
        cap_y = dy - 1 if side == "w" else dy + 1
        cap_sq = index_square(dx, cap_y)
        b.pop(cap_sq, None)
    b.pop(dst, None)
    b[dst] = piece
    return b


def legal_candidate(board: Dict[str, str], side: str, src: str, dst: str, ep: str, capture: bool) -> bool:
    piece = board[src]
    target = board.get(dst)
    if own(target, side):
        return False
    px = square_index(src)
    dx = square_index(dst)
    x1, y1 = px
    x2, y2 = dx
    ax, ay = abs(x2 - x1), abs(y2 - y1)
    p = piece.upper()
    if p == "P":
        direction = 1 if side == "w" else -1
        start_rank = 1 if side == "w" else 6
        if capture:
            if ay != 1 or ax != 1:
                return False
            if y2 - y1 != direction:
                return False
            if target is None and dst != ep:
                return False
            if target is not None and not enemy(target, side):
                return False
        else:
            if ax != 0 or target is not None:
                return False
            if y2 - y1 == direction:
                pass
            elif y1 == start_rank and y2 - y1 == 2 * direction:
                mid = index_square(x1, y1 + direction)
                if mid in board:
                    return False
            else:
                return False
    else:
        if capture and target is None:
            return False if dst != ep else False
        if not capture and target is not None:
            return False
        if not pseudo_attacks(board, src, dst):
            return False
    b2 = make_move(board, side, src, dst, ep)
    king = king_square(b2, side)
    return king is not None and not attacked(b2, king, "b" if side == "w" else "w")


def san_to_uci(fen: str, san: str) -> str:
    board, side, ep = board_from_fen(fen)
    s = san.strip().replace(" ", "")
    s = s.replace("0-0-0", "O-O-O").replace("0-0", "O-O")
    s = s.replace("e.p.", "")
    # Historical EPD typo used in Kaufman: Bcx3+ means Bxc3+.
    m_typo = re.fullmatch(r"([KQRBN])([a-h])x([1-8])([+#]?)", s)
    if m_typo:
        s = m_typo.group(1) + "x" + m_typo.group(2) + m_typo.group(3) + m_typo.group(4)
    s = re.sub(r"[!?]+$", "", s)
    s = re.sub(r"[+#]+$", "", s)

    # Castling is not used in this first 100-position set, but keep support.
    if s in ("O-O", "O-O-O"):
        rank = "1" if side == "w" else "8"
        src = "e" + rank
        dst = "g" + rank if s == "O-O" else "c" + rank
        return src + dst

    promo = ""
    pm = re.search(r"=([QRBN])$", s)
    if pm:
        promo = pm.group(1).lower()
        s = s[:pm.start()]

    if len(s) < 2:
        raise ValueError(f"Bad SAN {san!r}")
    dst = s[-2:]
    if dst[0] not in FILES or dst[1] not in RANKS:
        raise ValueError(f"Bad SAN destination {san!r}")
    prefix = s[:-2]
    capture = "x" in prefix
    prefix = prefix.replace("x", "")
    piece_type = "P"
    if prefix and prefix[0] in PIECE_TYPES:
        piece_type = prefix[0]
        disamb = prefix[1:]
    else:
        disamb = prefix

    candidates: List[str] = []
    for src, piece in board.items():
        if piece_color(piece) != side or piece.upper() != piece_type:
            continue
        if disamb:
            if len(disamb) == 1 and disamb in FILES and src[0] != disamb:
                continue
            if len(disamb) == 1 and disamb in RANKS and src[1] != disamb:
                continue
            if len(disamb) == 2 and src != disamb:
                continue
        if legal_candidate(board, side, src, dst, ep, capture):
            candidates.append(src)

    if len(candidates) != 1:
        raise ValueError(f"SAN {san!r} resolved to {len(candidates)} legal sources: {candidates}")
    uci = candidates[0] + dst + promo
    return uci


def validate_tests() -> None:
    if len(TESTS) != 100:
        raise AssertionError(f"Expected exactly 100 tests, got {len(TESTS)}")
    seen = set()
    for t in TESTS:
        if t.name in seen:
            raise AssertionError(f"Duplicate test name: {t.name}")
        seen.add(t.name)
        nf = normalize_position_fen(t.fen)
        for san in t.best_moves:
            san_to_uci(nf, san)


# ============================================================================
# UCI ENGINE DRIVER
# ============================================================================

class UCIEngine:
    def __init__(self, path: str):
        self.path = path
        self.proc: Optional[subprocess.Popen[str]] = None
        self.lines: "queue.Queue[str]" = queue.Queue()
        self.reader: Optional[threading.Thread] = None

    def start(self) -> None:
        self.proc = subprocess.Popen(
            [self.path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )
        assert self.proc.stdout is not None

        def pump() -> None:
            assert self.proc is not None
            for line in self.proc.stdout:
                self.lines.put(line.rstrip("\r\n"))

        self.reader = threading.Thread(target=pump, daemon=True)
        self.reader.start()
        self.send("uci")
        self.wait_for_token("uciok", 10.0)
        self.send("setoption name UCI_AnalyseMode value true")
        self.send("isready")
        self.wait_for_token("readyok", 10.0)

    def send(self, command: str) -> None:
        if self.proc is None or self.proc.stdin is None:
            raise RuntimeError("Engine not started")
        self.proc.stdin.write(command + "\n")
        self.proc.stdin.flush()

    def wait_for_token(self, token: str, timeout: float) -> List[str]:
        deadline = time.monotonic() + timeout
        seen: List[str] = []
        while time.monotonic() < deadline:
            try:
                line = self.lines.get(timeout=max(0.01, min(0.25, deadline - time.monotonic())))
            except queue.Empty:
                continue
            seen.append(line)
            if token in line:
                return seen
        raise TimeoutError(f"Timed out waiting for {token!r}. Output: {seen[-10:]}")

    def search(self, fen: str, depth: Optional[int], movetime: Optional[int], timeout: float) -> Tuple[str, Dict[str, str]]:
        self.send("stop")
        self.send("ucinewgame")
        self.send("position fen " + normalize_position_fen(fen))
        if movetime is not None:
            self.send(f"go movetime {movetime}")
        elif depth is not None:
            self.send(f"go depth {depth}")
        else:
            self.send("go depth 18")

        deadline = time.monotonic() + timeout
        last_info: Dict[str, str] = {}
        while time.monotonic() < deadline:
            try:
                line = self.lines.get(timeout=max(0.01, min(0.25, deadline - time.monotonic())))
            except queue.Empty:
                continue
            if line.startswith("info "):
                self._parse_info(line, last_info)
            if line.startswith("bestmove "):
                parts = line.split()
                return (parts[1] if len(parts) > 1 else "(none)", last_info)
        raise TimeoutError("Search timeout")

    @staticmethod
    def _parse_info(line: str, out: Dict[str, str]) -> None:
        toks = line.split()
        i = 1
        while i < len(toks):
            k = toks[i]
            if k in {"depth", "seldepth", "nodes", "nps", "time", "multipv"} and i + 1 < len(toks):
                out[k] = toks[i + 1]
                i += 2
            elif k == "score" and i + 2 < len(toks):
                out["score_type"] = toks[i + 1]
                out["score"] = toks[i + 2]
                i += 3
            else:
                i += 1

    def stop(self) -> None:
        try:
            self.send("quit")
        except Exception:
            pass
        if self.proc is not None:
            try:
                self.proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.proc.kill()


# ============================================================================
# REPORTING
# ============================================================================

CATEGORY_NAMES = {
    "TACTICA_BK": "Táctica / cálculo (Bratko-Kopec)",
    "ESTRATEGIA_KAUFMAN": "Estrategia / precisión posicional (Kaufman)",
    "CALCULO_NOLOT": "Cálculo táctico difícil (Nolot)",
    "POSICIONAL_LCT2": "Posicional (LCT-II)",
    "COMBINACIONES_LCT2": "Combinaciones (LCT-II)",
    "FINALES_RYBKA": "Finales / precisión técnica (Rybka Forum)",
    "FINALES_LCT2": "Finales / precisión técnica (LCT-II)",
}


def find_engine(path: str) -> str:
    if os.path.isfile(path):
        return os.path.abspath(path)
    found = shutil.which(path)
    if found:
        return found
    raise FileNotFoundError(f"Engine not found: {path}")


def print_header(args: argparse.Namespace) -> None:
    print("=" * 88)
    print("100-POSITION ENGINE REGRESSION TEST")
    print("=" * 88)
    print(f"Engine     : {args.engine}")
    if args.movetime is not None:
        print(f"Mode       : movetime {args.movetime} ms")
    else:
        print(f"Mode       : depth {args.depth}")
    print(f"Timeout    : {args.timeout:.1f} s/position")
    print("Criterion  : EXACT top-1 match against EPD bm")
    print("=" * 88)


def main() -> int:
    parser = argparse.ArgumentParser(description="Run 100 strict EPD best-move tests")
    parser.add_argument("engine", nargs="?", default="nanobit.exe", help="UCI engine executable")
    parser.add_argument("--depth", type=int, default=18, help="Fixed search depth (default: 18)")
    parser.add_argument("--movetime", type=int, default=None, help="Time per position in ms; overrides depth")
    parser.add_argument("--timeout", type=float, default=30.0, help="Hard timeout per position")
    parser.add_argument("--hash", type=int, default=None, help="Hash size in MB, if engine supports UCI Hash")
    parser.add_argument("--threads", type=int, default=None, help="Threads, if engine supports UCI Threads")
    parser.add_argument("--category", default=None, help="Run only one category key")
    parser.add_argument("--start", type=int, default=1, help="1-based test number to start from")
    parser.add_argument("--count", type=int, default=100, help="Number of tests to run")
    args = parser.parse_args()

    validate_tests()

    selected = TESTS
    if args.category:
        if args.category not in CATEGORY_NAMES:
            print("Unknown category. Available:")
            for k, v in CATEGORY_NAMES.items():
                print(f"  {k:24} {v}")
            return 2
        selected = [t for t in selected if t.category == args.category]
    else:
        begin = max(0, args.start - 1)
        selected = selected[begin:begin + args.count]

    engine_path = find_engine(args.engine)
    print_header(args)
    print(f"Tests      : {len(selected)}")
    print()

    eng = UCIEngine(engine_path)
    eng.start()
    if args.hash is not None:
        eng.send(f"setoption name Hash value {args.hash}")
    if args.threads is not None:
        eng.send(f"setoption name Threads value {args.threads}")
    eng.send("isready")
    eng.wait_for_token("readyok", 10.0)

    results: List[Tuple[TestCase, str, str, bool, Dict[str, str]]] = []
    category_totals: Dict[str, List[int]] = {}
    try:
        for i, t in enumerate(selected, 1):
            expected_uci = {san_to_uci(normalize_position_fen(t.fen), san): san for san in t.best_moves}
            expected_text = "/".join(expected_uci.keys())
            t0 = time.perf_counter()
            try:
                got, info = eng.search(t.fen, args.depth, args.movetime, args.timeout)
                ok = got.lower() in {u.lower() for u in expected_uci}
                elapsed = time.perf_counter() - t0
                status = "PASS" if ok else "MISS"
                print(f"[{i:03d}/{len(selected):03d}] {status:4} {t.name:11} {t.category:22} got={got:6} exp={expected_text:13} {elapsed:6.2f}s")
            except Exception as exc:
                got = "ERROR"
                ok = False
                info = {}
                elapsed = time.perf_counter() - t0
                print(f"[{i:03d}/{len(selected):03d}] ERROR {t.name:11} {t.category:22} {exc} ({elapsed:.2f}s)")
            results.append((t, got, expected_text, ok, info))
            ct = category_totals.setdefault(t.category, [0, 0])
            ct[0] += int(ok)
            ct[1] += 1
    finally:
        eng.stop()

    print()
    print("-" * 88)
    print("CATEGORY RESULTS")
    print("-" * 88)
    total_ok = sum(int(r[3]) for r in results)
    for cat, (ok, total) in category_totals.items():
        print(f"{cat:24} {ok:3}/{total:<3}  {100.0 * ok / max(total,1):6.2f}%")
    print("-" * 88)
    print(f"TOTAL                    {total_ok:3}/{len(results):<3}  {100.0 * total_ok / max(len(results),1):6.2f}%")
    print()

    misses = [r for r in results if not r[3]]
    if misses:
        print("FAILURES / POSITIONS TO INVESTIGATE")
        print("-" * 88)
        for t, got, exp, _, info in misses:
            score = ""
            if info.get("score_type"):
                score = f" score={info.get('score_type')}:{info.get('score')}"
            print(f"{t.name:11} {t.category:24} got={got:8} expected={exp:16}{score}")
        print()

    return 0 if total_ok == len(results) else 1


if __name__ == "__main__":
    try:
        main_rc = main()
    except KeyboardInterrupt:
        print("\nInterrupted.")
        main_rc = 130
    except Exception as exc:
        print(f"FATAL: {exc}", file=sys.stderr)
        main_rc = 2
    raise SystemExit(main_rc)
