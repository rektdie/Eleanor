#include "perft.h"
#include "stopwatch.h"
#include "movegen.h"
#include <cassert>

static U64 HelperPerft(Board &board, int depth) {
    MOVEGEN::GenerateMoves<All>(board, true);

    if (depth == 0) return 1ULL;

    U64 nodes = 0;

    for (int i = 0; i < board.currentMoveIndex; i++) {
        if (!board.IsLegal(board.moveList[i])) continue;

        Board copy = board;
        copy.MakeMove(board.moveList[i]);

        nodes += HelperPerft(copy, depth - 1);
    }
    return nodes;
}

void Perft(Board &board, int depth) {
    //board.SetByFen(StartingFen);
    MOVEGEN::GenerateMoves<All>(board, true);

    U64 totalNodes = 0;

    Stopwatch sw;
    for (int i = 0; i < board.currentMoveIndex; i++) {
        if (!board.IsLegal(board.moveList[i])) continue;
        Board copy = board;
        copy.MakeMove(board.moveList[i]);
        U64 nodeCount = HelperPerft(copy, depth - 1);
        totalNodes += nodeCount;
        board.moveList[i].PrintMove();

        std::cout << ": " << nodeCount << std::endl;
    }

    std::cout << std::endl << "Depth: " << depth << std::endl;
    std::cout << "Total nodes: " << totalNodes << std::endl;
    std::cout << "Time took: " << sw.GetElapsedSec() << "s (";
    std::cout << int(totalNodes / sw.GetElapsedSec()) << " nodes/sec)" << std::endl;
}
