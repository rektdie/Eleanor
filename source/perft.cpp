#include "perft.h"
#include "stopwatch.h"

U64 HelperPerft(Board board, int depth) {
    GenerateMoves(board, board.sideToMove);

    if (depth == 1) return board.currentMoveIndex;
    if (depth == 0) return 1ULL;

    U64 nodes = 0;

    for (int i = 0; i < board.currentMoveIndex; i++) {
        Board copy = board;
        board.MakeMove(board.moveList[i]);
        nodes += HelperPerft(board, depth - 1);
        board = copy;
    }
    return nodes;
}

void Perft(Board &board, int depth) {
    GenerateMoves(board, board.sideToMove);

    U64 totalNodes = 0;

    Stopwatch sw;
    for (int i = 0; i < board.currentMoveIndex; i++) {
        Board copy = board;
        board.MakeMove(board.moveList[i]);
        U64 nodeCount = HelperPerft(board, depth - 1);
        board = copy;
        totalNodes += nodeCount;
        board.moveList[i].PrintMove();

        std::cout << ": " << nodeCount << '\n';
    }

    std::cout << "\nDepth: " << depth << '\n';
    std::cout << "Total nodes: " << totalNodes << '\n';
    std::cout << "Time took: " << sw.GetElapsedSec() << "s (";
    std::cout << int(totalNodes / sw.GetElapsedSec()) << " nodes/sec)\n";
}
