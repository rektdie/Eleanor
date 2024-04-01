#include <chrono>
#include "perft.h"

static int captures = 0;
static int epCaptures = 0;
static int castles = 0;
static int promotions = 0;
static int checks = 0;


U64 HelperPerft(Board board, int depth) {
    if (depth == 0) return 1ULL;

    GenerateMoves(board, board.sideToMove);

    U64 nodes = 0;

    for (int i = 0; i < board.moveList.size(); i++) {
        if (board.moveList[i].moveFlags == capture) captures++;
        if (board.moveList[i].moveFlags == epCapture) epCaptures++;
        if (board.moveList[i].moveFlags == kingCastle || board.moveList[i].moveFlags == queenCastle) castles++;
        if (board.moveList[i].moveFlags >= knightPromotion) promotions++;
        board.MakeMove(board.moveList[i]);
        if (board.InCheck(board.sideToMove)) checks++;
        nodes += HelperPerft(board, depth - 1);
        board.UnmakeMove();
    }
    return nodes;
}

void Perft(Board &board, int depth) {
    using std::chrono::high_resolution_clock;
    using std::chrono::milliseconds;
    using std::chrono::duration_cast;

    captures = 0;
    epCaptures = 0;
    castles = 0;
    promotions = 0;
    checks = 0;

    auto start = high_resolution_clock::now();
    U64 nodes = HelperPerft(board, depth);
    auto end = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(end - start);

    std::cout << "Depth: " << depth << '\n';
    std::cout << "Nodes searched: " << nodes << '\n';
    std::cout << "Captures: " << captures << '\n';
    std::cout << "epCaptures: " << epCaptures << '\n';
    std::cout << "Castles: " << castles << '\n';
    std::cout << "Promotions: " << promotions << '\n';
    std::cout << "Checks: " << checks << '\n' << '\n';
    std::cout << "Total time taken: " << duration.count() << " ms";
    std::cout << " (" << (nodes / duration.count())  * 1000 << " nodes/sec)\n";
}