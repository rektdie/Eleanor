#include "perft.h"
#include "stopwatch.h"
#include "movegen.h"

static U64 HelperPerft(Board board, int depth) {
    MOVEGEN::GenerateMoves(board);

    if (depth == 1) return board.currentMoveIndex;
    if (depth == 0) return 1ULL;

    U64 nodes = 0;

    for (int i = 0; i < board.currentMoveIndex; i++) {
        Board copy = board;
        board.MakeMove(board.moveList[i]);
        nodes += HelperPerft(board, depth - 1);
        positionIndex--;
        board = copy;
    }
    return nodes;
}

void Perft(Board &board, int depth) {
    MOVEGEN::GenerateMoves(board);

    U64 totalNodes = 0;

    Stopwatch sw;
    for (int i = 0; i < board.currentMoveIndex; i++) {
        Board copy = board;
        board.MakeMove(board.moveList[i]);
        U64 nodeCount = HelperPerft(board, depth - 1);
        positionIndex--;
        board = copy;
        totalNodes += nodeCount;
        board.moveList[i].PrintMove();

        std::cout << ": " << nodeCount << std::endl;
    }

    std::cout << std::endl << "Depth: " << depth << std::endl;
    std::cout << "Total nodes: " << totalNodes << std::endl;
    std::cout << "Time took: " << sw.GetElapsedSec() << "s (";
    std::cout << int(totalNodes / sw.GetElapsedSec()) << " nodes/sec)" << std::endl;
}
