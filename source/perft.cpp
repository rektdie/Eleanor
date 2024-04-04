// #include <chrono>
// #include "perft.h"

// U64 HelperPerft(Board board, int depth) {
//     GenerateMoves(board, board.sideToMove);

//     if (depth == 1) return board.moveList.size();
//     if (depth == 0) return 1ULL;

//     U64 nodes = 0;

//     for (int i = 0; i < board.moveList.size(); i++) {
//         board.MakeMove(board.moveList[i]);
//         nodes += HelperPerft(board, depth - 1);
//         board.UnmakeMove();
//     }
//     return nodes;
// }

// void Perft(Board &board, int depth) {
//     GenerateMoves(board, board.sideToMove);

//     U64 totalNodes = 0;

//     for (int i = 0; i < board.moveList.size(); i++) {
//         board.MakeMove(board.moveList[i]);
//         U64 nodeCount = HelperPerft(board, depth - 1);
//         totalNodes += nodeCount;
//         board.moveList[i].PrintMove();
//         board.UnmakeMove();
//         std::cout << ": " << nodeCount << '\n';
//     }

//     auto start = std::chrono::steady_clock::now();
//     U64 nodes = HelperPerft(board, depth);
//     auto finish = std::chrono::steady_clock::now();

//     double elapsed_seconds = std::chrono::duration_cast<
//         std::chrono::duration<double>>(finish - start).count();

//     std::cout << "\nDepth: " << depth << '\n';
//     std::cout << "Total nodes: " << totalNodes << '\n';
//     std::cout << "Time took: " << elapsed_seconds << "s (";
//     std::cout << nodes / elapsed_seconds << " nodes/sec)\n";
// }