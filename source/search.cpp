#include "search.h"
#include "movegen.h"
#include "evaluate.h"
#include <algorithm>
#include "tt.h"

static inline int nodes = 0;
static inline int ply = 0;

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

static SearchResults Quiescence(Board board, int alpha, int beta) {
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

    SearchResults results;

    for (int i = 0; i < board.currentMoveIndex; i++) {
        if (board.moveList[i].IsCapture()) {
            Board copy = board;
            copy.MakeMove(board.moveList[i]);
            int score = -Quiescence(copy, -beta, -alpha).score;

            if (score >= beta) {
                return beta;
            }

            if (score > alpha) {
                alpha = score;
                results.bestMove = board.moveList[i];
            }
        }
    }

    results.score = alpha;
    return results;
}

static SearchResults PVS(Board board, int depth, int alpha, int beta) {
    nodes++;
    if (depth == 0) return Quiescence(board, alpha, beta).score;

    int score = -50000;

    GenerateMoves(board, board.sideToMove);
    SortMoves(board);

    if (board.InCheck(board.sideToMove)) depth++;

    if (board.currentMoveIndex == 0) {
        if (board.InCheck(board.sideToMove)) { // checkmate
            return -49000 + ply;
        } else { // stalemate
            return 0;
        }
    }

    SearchResults results;

    // For all moves
    for (int i = 0; i < board.currentMoveIndex; i++) {
        Board copy = board;
        copy.MakeMove(copy.moveList[i]);

        // First move (suspected PV node)
        if (!i) {
            // Full search
            ply++;
            score = -PVS(copy, depth - 1, -beta, -alpha).score;
            ply--;
        } else {
            // Quick search
            ply++;
            score = -PVS(copy, depth - 1, -alpha-1, -alpha).score;
            ply--;

            if (score > alpha && beta - alpha > 1) {
                // We have to do full search
                ply++;
                score = -PVS(copy, depth - 1, -beta, -alpha).score;
                ply--;
            }
        }

        if (score >= beta) {
            return beta;
        }

        if (score > alpha) {
            alpha = score;
            results.bestMove = board.moveList[i];
        }
    }

    results.score = alpha;
    return results;
}

void SearchPosition(Board &board, int depth) {
    SearchResults results = PVS(board, depth, -50000, 50000);
    std::cout << "info depth " << depth << " nodes " << nodes << '\n'; 
    std::cout << "bestmove " << squareCoords[results.bestMove.MoveFrom()]
        << squareCoords[results.bestMove.MoveTo()] <<  '\n';
    nodes = 0;
}
