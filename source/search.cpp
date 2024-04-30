#include "search.h"
#include "movegen.h"
#include "evaluate.h"

int NegaMax(Board board, int depth, int alpha, int beta) {
    if (depth == 0) return Evaluate(board);

    GenerateMoves(board, board.sideToMove);

    int max = -50000000;

    for (Move move : board.moveList) {
        Board copy = board;
        board.MakeMove(move);
        int score = -NegaMax(board, depth - 1, -beta, -alpha);
        board = copy;

        if (score >= beta) {
            return beta;
        }

        if (score > alpha) {
            alpha = score;
        }

        if (score > max) {
            max = score;
        }
    }

    return max;
}

Move NegaMaxHandler(Board &board, int depth) {
    Move bestMove = Move();

    GenerateMoves(board, board.sideToMove);

    int max = -50000000;

    for (Move move : board.moveList) {
        Board copy = board;
        board.MakeMove(move);
        int score = -NegaMax(board, depth - 1, -5000000, 5000000);
        board = copy;

        if (score > max) {
            max = score;
            bestMove = move;
        }
    }

    return bestMove;
}

void SearchPosition(Board &board, int depth) {
    std::cout << "best move: ";
    NegaMaxHandler(board, depth).PrintMove();
}