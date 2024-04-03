#include <chrono>
#include "perft.h"

U64 HelperPerft(Board board, int depth) {
    GenerateMoves(board, board.sideToMove);

    if (depth == 1) return board.moveList.size();

    U64 nodes = 0;

    for (int i = 0; i < board.moveList.size(); i++) {
        board.MakeMove(board.moveList[i]);
        nodes += HelperPerft(board, depth - 1);
        board.UnmakeMove();
    }
    return nodes;
}

void Perft(Board &board, int depth) {
    using std::chrono::high_resolution_clock;
    using std::chrono::milliseconds;
    using std::chrono::duration_cast;

    GenerateMoves(board, board.sideToMove);

    int totalNodes = 0;

    for (int i = 0; i < board.moveList.size(); i++) {
        board.MakeMove(board.moveList[i]);
        int nodeCount = HelperPerft(board, depth - 1);
        totalNodes += nodeCount;
        board.moveList[i].PrintMove();
        board.UnmakeMove();
        std::cout << ": " << nodeCount << '\n';
    }

    auto start = high_resolution_clock::now();
    U64 nodes = HelperPerft(board, depth);
    auto end = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(end - start);

    std::cout << "\nDepth: " << depth << '\n';
    std::cout << "Total nodes: " << totalNodes << '\n';
    std::cout << "Total time taken: " << duration.count() << " ms";
    std::cout << " (" << (nodes / duration.count())  * 1000 << " nodes/sec)\n";
}