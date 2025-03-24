#include "perft.h"
#include "stopwatch.h"
#include "movegen.h"

static U64 HelperPerft(Board board, int depth) {
    MOVEGEN::GenerateMoves<All>(board);

    if (depth == 1) return board.currentMoveIndex;
    if (depth == 0) return 1ULL;

    U64 nodes = 0;

    for (int i = 0; i < board.currentMoveIndex; i++) {
        Board copy = board;
        copy.MakeMove(board.moveList[i]);
        nodes += HelperPerft(copy, depth - 1);
        positionIndex--;
    }
    return nodes;
}

void Perft(Board &board, int depth) {
    MOVEGEN::GenerateMoves<All>(board);

    U64 totalNodes = 0;

    Stopwatch sw;
    for (int i = 0; i < board.currentMoveIndex; i++) {
        Board copy = board;
        copy.MakeMove(board.moveList[i]);
        U64 nodeCount = HelperPerft(copy, depth - 1);
        positionIndex--;
        totalNodes += nodeCount;
        board.moveList[i].PrintMove();

        std::cout << ": " << nodeCount << std::endl;
    }

    std::cout << std::endl << "Depth: " << depth << std::endl;
    std::cout << "Total nodes: " << totalNodes << std::endl;
    std::cout << "Time took: " << sw.GetElapsedSec() << "s (";
    std::cout << int(totalNodes / sw.GetElapsedSec()) << " nodes/sec)" << std::endl;
}
