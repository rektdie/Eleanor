#include <chrono>
#include "perft.h"

U64 HelperPerft(Board board, int depth) {
    GenerateMoves(board, board.sideToMove);

    if (depth == 1) return board.moveList.size();
    if (depth == 0) return 1ULL;

    U64 nodes = 0;

    for (int i = 0; i < board.moveList.size(); i++) {
        board.MakeMove(board.moveList[i]);
        nodes += HelperPerft(board, depth - 1);
        board.UnmakeMove();
    }
    return nodes;
}

void Perft(Board &board, int depth) {
    GenerateMoves(board, board.sideToMove);

    U64 totalNodes = 0;

    for (int i = 0; i < board.moveList.size(); i++) {
        board.MakeMove(board.moveList[i]);
        U64 nodeCount = HelperPerft(board, depth - 1);
        totalNodes += nodeCount;
        board.moveList[i].PrintMove();
        board.UnmakeMove();
        std::cout << ": " << nodeCount << '\n';
    }

    U64 nodes = HelperPerft(board, depth);

    std::cout << "\nDepth: " << depth << '\n';
    std::cout << "Total nodes: " << totalNodes << '\n';
}