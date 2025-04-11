#include <cstring>
#include <string_view>
#include <array>
#include "benchmark.h"
#include "search.h"
#include "stopwatch.h"
#include "tt.h"

using namespace SEARCH;

constexpr std::array<std::string_view, 50> fenPositions = {
    "rnbq1k1r/ppp1bppp/4pn2/8/2B5/2NP1N2/PPP2PPP/R1BQR1K1 b - - 2 8",
    "rnbq1k1r/pp2bppp/4pn2/2p5/2B2B2/2NP1N2/PPP2PPP/R2QR1K1 b - - 1 9",
    "r1bq1k1r/pp2bppp/2n1pn2/2p5/2B1NB2/3P1N2/PPP2PPP/R2QR1K1 b - - 3 10",
    "r1bq1k1r/pp2bppp/2n1p3/2p5/2B1PB2/5N2/PPP2PPP/R2QR1K1 b - - 0 11",
    "r1b2k1r/pp2bppp/2n1p3/2p5/2B1PB2/5N2/PPP2PPP/3RR1K1 b - - 0 12",
    "r1b1k2r/pp2bppp/2n1p3/2p5/2B1PB2/2P2N2/PP3PPP/3RR1K1 b - - 0 13",
    "r1b1k2r/1p2bppp/p1n1p3/2p5/4PB2/2P2N2/PP2BPPP/3RR1K1 b - - 1 14",
    "r1b1k2r/4bppp/p1n1p3/1pp5/P3PB2/2P2N2/1P2BPPP/3RR1K1 b - - 0 15",
    "r1b1k2r/4bppp/p1n1p3/1P6/2p1PB2/2P2N2/1P2BPPP/3RR1K1 b - - 0 16",
    "r1b1k2r/4bppp/2n1p3/1p6/2p1PB2/1PP2N2/4BPPP/3RR1K1 b - - 0 17",
    "r3k2r/3bbppp/2n1p3/1p6/2P1PB2/2P2N2/4BPPP/3RR1K1 b - - 0 18",
    "r3k2r/3bbppp/2n1p3/8/1pP1P3/2P2N2/3BBPPP/3RR1K1 b - - 1 19",
    "1r2k2r/3bbppp/2n1p3/8/1pPNP3/2P5/3BBPPP/3RR1K1 b - - 3 20",
    "1r2k2r/3bbppp/2n1p3/8/2PNP3/2B5/4BPPP/3RR1K1 b - - 0 21",
    "1r2k2r/3bb1pp/2n1pp2/1N6/2P1P3/2B5/4BPPP/3RR1K1 b - - 1 22",
    "1r2k2r/3b2pp/2n1pp2/1N6/1BP1P3/8/4BPPP/3RR1K1 b - - 0 23",
    "1r2k2r/3b2pp/4pp2/1N6/1nP1P3/8/3RBPPP/4R1K1 b - - 1 24",
    "1r5r/3bk1pp/4pp2/1N6/1nP1PP2/8/3RB1PP/4R1K1 b - - 0 25",
    "1r5r/3bk1pp/2n1pp2/1N6/2P1PP2/8/3RBKPP/4R3 b - - 2 26",
    "1r5r/3bk1pp/2n2p2/1N2p3/2P1PP2/6P1/3RBK1P/4R3 b - - 0 27",
    "1r1r4/3bk1pp/2n2p2/1N2p3/2P1PP2/6P1/3RBK1P/R7 b - - 2 28",
    "1r1r4/N3k1pp/2n1bp2/4p3/2P1PP2/6P1/3RBK1P/R7 b - - 4 29",
    "1r1r4/3bk1pp/2N2p2/4p3/2P1PP2/6P1/3RBK1P/R7 b - - 0 30",
    "1r1R4/4k1pp/2b2p2/4p3/2P1PP2/6P1/4BK1P/R7 b - - 0 31",
    "3r4/4k1pp/2b2p2/4P3/2P1P3/6P1/4BK1P/R7 b - - 0 32",
    "3r4/R3k1pp/2b5/4p3/2P1P3/6P1/4BK1P/8 b - - 1 33",
    "8/3rk1pp/2b5/R3p3/2P1P3/6P1/4BK1P/8 b - - 3 34",
    "8/3r2pp/2bk4/R1P1p3/4P3/6P1/4BK1P/8 b - - 0 35",
    "8/2kr2pp/2b5/R1P1p3/4P3/4K1P1/4B2P/8 b - - 2 36",
    "1k6/3r2pp/2b5/RBP1p3/4P3/4K1P1/7P/8 b - - 4 37",
    "8/1k1r2pp/2b5/R1P1p3/4P3/3BK1P1/7P/8 b - - 6 38",
    "1k6/3r2pp/2b5/2P1p3/4P3/3BK1P1/7P/R7 b - - 8 39",
    "1k6/r5pp/2b5/2P1p3/4P3/3BK1P1/7P/5R2 b - - 10 40",
    "1k3R2/6pp/2b5/2P1p3/4P3/r2BK1P1/7P/8 b - - 12 41",
    "5R2/2k3pp/2b5/2P1p3/4P3/r2B2P1/3K3P/8 b - - 14 42",
    "5R2/2k3pp/2b5/2P1p3/4P3/3BK1P1/r6P/8 b - - 16 43",
    "5R2/2k3pp/2b5/2P1p3/4P3/r2B2P1/4K2P/8 b - - 18 44",
    "5R2/2k3pp/2b5/2P1p3/4P3/3B1KP1/r6P/8 b - - 20 45",
    "8/2k2Rpp/2b5/2P1p3/4P3/r2B1KP1/7P/8 b - - 22 46",
    "3k4/5Rpp/2b5/2P1p3/4P3/r2B2P1/4K2P/8 b - - 24 47",
    "3k4/5Rpp/2b5/2P1p3/4P3/3B1KP1/r6P/8 b - - 26 48",
    "3k4/5Rpp/2b5/2P1p3/4P3/r2B2P1/4K2P/8 b - - 28 49",
    "3k4/5Rpp/2b5/2P1p3/4P3/3BK1P1/r6P/8 b - - 30 50",
    "3k4/5Rpp/2b5/2P1p3/4P3/r2B2P1/3K3P/8 b - - 32 51",
    "3k4/5Rpp/2b5/2P1p3/4P3/2KB2P1/r6P/8 b - - 34 52",
    "3k4/5Rpp/2b5/2P1p3/4P3/r2B2P1/2K4P/8 b - - 36 53",
    "3k4/5Rpp/2b5/2P1p3/4P3/1K1B2P1/r6P/8 b - - 38 54",
    "3k4/6Rp/2b5/2P1p3/4P3/1K1B2P1/7r/8 b - - 0 55",
    "3k4/8/2b3Rp/2P1p3/4P3/1K1B2P1/7r/8 b - - 1 56",
    "8/2k3R1/2b4p/2P1p3/4P3/1K1B2P1/7r/8 b - - 3 57",
};

void RunBenchmark() {
    Board board;
    int depth = BENCH_DEPTH;
    int ply = 0;
    searchStopped = false;
    positionIndex = 0;

    // Clearing
    TT.Clear();
    std::memset(killerMoves, 0, sizeof(killerMoves));

    SearchContext ctx;


    Stopwatch sw;
    for (int i = 0; i < fenPositions.size(); i++) {
        board.SetByFen(fenPositions[i]);
        SearchPosition<bench>(board, SearchParams(), ctx);
    }

    std::cout << ctx.nodes << " nodes " << int(ctx.nodes/sw.GetElapsedSec()) << " nps" << std::endl;
    TT.Clear();
    std::memset(killerMoves, 0, sizeof(killerMoves));
    positionIndex = 0;
}
