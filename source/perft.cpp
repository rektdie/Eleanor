#include <chrono>
#include "perft.h"

U64 HelperPerft(Board board, int depth) {
    GenerateMoves(board);

    if (depth == 1) return board.currentMoveIndex;
    if (depth == 0) return 1ULL;

    U64 nodes = 0;

    for (int i = 0; i < board.currentMoveIndex; i++) {
        Board copy = board;
        copy.MakeMove(board.moveList[i]);
        nodes += HelperPerft(copy, depth - 1);
    }
    return nodes;
}

void Perft(Board &board, int depth) {
    GenerateMoves(board);

    U64 totalNodes = 0;

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < board.currentMoveIndex; i++) {
        Board copy = board;
        copy.MakeMove(board.moveList[i]);
        U64 nodeCount = HelperPerft(copy, depth - 1);
        totalNodes += nodeCount;
        std::cout << squareCoords[board.moveList[i].MoveFrom()]
            << squareCoords[board.moveList[i].MoveTo()];
        
        // Promotions
        int flags = board.moveList[i].GetFlags();
        if (flags == knightPromotion || flags == knightPromoCapture) {
            std::cout << 'n';
        } else if (flags == bishopPromotion || flags == bishopPromoCapture) {
            std::cout << 'b';
        } else if (flags == rookPromotion || flags == rookPromoCapture) {
            std::cout << 'r';
        } else if (flags == queenPromotion || flags == queenPromoCapture) {
            std::cout << 'q';
        }

        std::cout << ": " << nodeCount << '\n';
    }
    auto finish = std::chrono::steady_clock::now();

    double elapsed_seconds = std::chrono::duration_cast<
        std::chrono::duration<double>>(finish - start).count();

    std::cout << "\nDepth: " << depth << '\n';
    std::cout << "Total nodes: " << totalNodes << '\n';
    std::cout << "Time took: " << elapsed_seconds << "s (";
    std::cout << int(totalNodes / elapsed_seconds) << " nodes/sec)\n";
}
