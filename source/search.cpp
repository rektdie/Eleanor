#include "search.h"
#include "movegen.h"
#include "evaluate.h"
#include <algorithm>
#include "tt.h"

static inline int nodes = 0;
static inline Move bestMove = Move();
static inline int bestScore = -50000;

static int ScoreMove(Board &board, Move &move) {
    const int attackerType = board.GetPieceType(move.MoveFrom());
    const int targetType = board.GetPieceType(move.MoveTo());

    if (move.IsCapture()) {
        return moveScoreTable[attackerType][targetType] + 10000;
    }

    return 0;
}

static void SortMoves(Board &board) {
    std::array<int, 218> scores;
    std::array<int, 218> indices;

    // Initialize scores and indices
    for (int i = 0; i < board.currentMoveIndex; i++) {
        scores[i] = ScoreMove(board, board.moveList[i]);
        indices[i] = i;
    }

    // Sort indices based on scores
    std::sort(indices.begin(), indices.begin() + board.currentMoveIndex,
              [&scores](int a, int b) {
                  return scores[a] > scores[b];
              });

    // Create a temporary move list and reorder the moveList based on the sorted indices
    std::array<Move, 218> sortedMoves;
    for (int i = 0; i < board.currentMoveIndex; i++) {
        sortedMoves[i] = board.moveList[indices[i]];
    }

    // Assign the sorted moves back to the board's move list
    for (int i = 0; i < board.currentMoveIndex; i++) {
        board.moveList[i] = sortedMoves[i];
    }
}

void ListScores(Board &board) {
    SortMoves(board);

    for (int i = 0; i < board.currentMoveIndex; i++) {
        Move currentMove = board.moveList[i];

        std::cout << squareCoords[currentMove.MoveFrom()] << squareCoords[currentMove.MoveTo()];
        std::cout << ": " << ScoreMove(board, currentMove) << '\n';
    }
}

static int Quiescence(Board board, int alpha, int beta) {
    nodes++;

    int staticScore = Evaluate(board);

    if (staticScore >= beta) {
        return beta;
    }

    if (staticScore > alpha) {
        alpha = staticScore;
    }

    GenerateMoves(board, board.sideToMove);

    SortMoves(board);

    for (int i = 0; i < board.currentMoveIndex; i++) {
        if (board.moveList[i].IsCapture()) {
            Board copy = board;
            board.MakeMove(board.moveList[i]);
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

static int PVS(Board board, int depth, int alpha, int beta, int isRoot) {
    nodes++;
    if (depth == 0) return Quiescence(board, alpha, beta);

    int score = -50000;

    GenerateMoves(board, board.sideToMove);
    SortMoves(board);

    // For all moves
    for (int i = 0; i < board.currentMoveIndex; i++) {
        Board copy = board;
        copy.MakeMove(copy.moveList[i]);

        // First move (suspected PV node)
        if (!i) {
            // Full search
            score = -PVS(copy, depth - 1, -beta, -alpha, false);
        } else {
            // Quick search
            score = -PVS(copy, depth - 1, -alpha-1, -alpha, false);

            if (score > alpha && beta - alpha > 1) {
                // We have to do full search
                score = -PVS(copy, depth - 1, -beta, -alpha, false);
            }
        }

        if (score >= beta) {
            return beta;
        }

        if (score > alpha) {
            alpha = score;
        }

        if (isRoot) {
            if (alpha > bestScore) {
                bestScore = alpha;
                bestMove = copy.moveList[i];
            }
        }
    }

    return alpha;
}

void SearchPosition(Board &board, int depth) {
    bestScore = -50000;
    bestMove = Move();
    PVS(board, depth, -50000, 50000, true);
    std::cout << "info depth " << depth << " nodes " << nodes << '\n'; 
    std::cout << "bestmove " << squareCoords[bestMove.MoveFrom()]
        << squareCoords[bestMove.MoveTo()] <<  '\n';
    nodes = 0;
}
