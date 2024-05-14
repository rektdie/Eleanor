#include "search.h"
#include "movegen.h"
#include "evaluate.h"

static inline int ply = 0;
static inline Move bestMove = Move();
static inline int bestScore = -50000;

int Quiescence(Board board, int alpha, int beta) {
    int staticScore = Evaluate(board);

    if (staticScore >= beta) {
        return beta;
    }

    if (staticScore > alpha) {
        alpha = staticScore;
    }

    GenerateMoves(board, board.sideToMove);

    for (Move move : board.moveList) {
        if (move.IsCapture()) {
            Board copy = board;
            board.MakeMove(move);
            int score = -Quiescence(board, -beta, -alpha);
            board = copy;

            if (score >= beta) {
                return beta;
            }

            if (score > alpha) {
                alpha = score;
            }
        }
    }

    return alpha;
}

int NegaMax(Board board, int depth, int alpha, int beta, bool isRoot) {
    if (depth == 0) {
        return Quiescence(board, -50000, 50000);
        //return Evaluate(board);
    }

    GenerateMoves(board, board.sideToMove);

    if (board.moveList.size() == 0) {
        if (board.InCheck(board.sideToMove)) { // checkmate
            return -49000 + ply;
        } else { // stalemate
            return 0;
        }
    }


    int max = -50000;

    for (Move move : board.moveList) {
        Board copy = board;
        board.MakeMove(move);
        ply++;
        int score = -NegaMax(board, depth - 1, -beta, -alpha, false);
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

        if (isRoot) {
            if (max > bestScore) {
                bestScore = max;
                bestMove = move;
            }
        }
    }
    return max;
}

void SearchPosition(Board &board, int depth) {
    bestScore = -50000;
    bestMove = Move();
    NegaMax(board, depth, -50000, 50000, true); 
    std::cout << "bestmove " << squareCoords[bestMove.MoveFrom()]
        << squareCoords[bestMove.MoveTo()] <<  '\n';
}
