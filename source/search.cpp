#include "search.h"
#include "movegen.h"
#include "evaluate.h"

static int ply = 0;

int NegaMax(Board board, int depth, int alpha, int beta) {
    GenerateMoves(board, board.sideToMove);
    if (board.moveList.size() == 0) {
        // Checkmate
        if (board.InCheck(board.sideToMove)) {
            return -4900000 + ply;
        } else { // Stalemate
            return 0;
        }
    }
    
    if (depth == 0) return Evaluate(board);

    int max = -5000000;

    for (Move move : board.moveList) {
        Board copy = board;
        board.MakeMove(move);
        ply++;
        int score = -NegaMax(board, depth - 1, -beta, -alpha);
        board = copy;
        ply--;

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

    int max = -5000000;

    for (Move move : board.moveList) {
        Board copy = board;
        board.MakeMove(move);
        ply++;
        int score = -NegaMax(board, depth - 1, -5000000, 5000000);
        board = copy;
        ply--;

        if (score > max) {
            max = score;
            bestMove = move;
        }
    }

    return bestMove;
}

void SearchPosition(Board &board, int depth) {
    Move bestMove = NegaMaxHandler(board, depth);
    std::cout << "bestmove " << squareCoords[bestMove.MoveFrom()]
        << squareCoords[bestMove.MoveTo()] <<  '\n';
}