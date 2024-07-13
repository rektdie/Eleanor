#include "search.h"
#include "movegen.h"
#include "evaluate.h"
#include <algorithm>
#include "tt.h"

static inline int nodes = 0;
static inline int ply = 0;
static inline Move bestMove = Move();
static inline int bestScore = -50000;

// killer moves [id][ply]
static inline Move killerMoves[2][64];

static int ScoreMove(Board &board, Move &move) {
    const int attackerType = board.GetPieceType(move.MoveFrom());
    const int targetType = board.GetPieceType(move.MoveTo());

    if (move.IsCapture()) {
        return moveScoreTable[attackerType][targetType] + 10000;
    } else {
        // 1st killer move
        if (killerMoves[0][ply] == move) {
            return 9000;
        }
        // 2nd killer move
        else if (killerMoves[1][ply] == move) {
            return 8000;
        }
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
    int score = ReadEntry(board.hashKey, ply, alpha, beta);

    if (score != 10000) {
        return score;
    }

    int nodeType = AllNode;

    nodes++;

    int staticScore = Evaluate(board);

    if (staticScore >= beta) {
        WriteEntry(board.hashKey, ply, beta, PV);
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
            score = -Quiescence(board, -beta, -alpha);
            board = copy;

            if (score >= beta) {
                WriteEntry(board.hashKey, ply, beta, CutNode);
                return beta;
            }

            if (score > alpha) {
                nodeType = PV;
                alpha = score;
            }
        }
    }

    WriteEntry(board.hashKey, ply, alpha, nodeType);

    return alpha;
}

static int NegaMax(Board board, int depth, int alpha, int beta, bool isRoot) {
    int score = ReadEntry(board.hashKey, depth, alpha, beta); 
    if (score != 10000) {
        return score;
    }

    int nodeType = AllNode;

    if (depth == 0) {
        score = Quiescence(board, alpha, beta);
        // Write to TT (PV)
        WriteEntry(board.hashKey, depth, score, PV);
        return score;
    }

    nodes++;

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

    for (int i = 0; i < board.currentMoveIndex; i++) {
        Board copy = board;
        board.MakeMove(board.moveList[i]);
        ply++;
        score = -NegaMax(board, depth - 1, -beta, -alpha, false);
        board = copy;
        ply--;

        if (score >= beta) {
            if (board.moveList[i].IsCapture()) {
                // store killer moves
                killerMoves[1][ply] = killerMoves[0][ply];
                killerMoves[0][ply] = board.moveList[i];
            }

            // Write to TT (CutNode)
            WriteEntry(board.hashKey, depth, beta, CutNode);
            return beta;
        }

        if (score > alpha) {
            alpha = score;

            // Write to TT (PV)
            // if this doesnt occur, then it remains AllNode
            nodeType = PV;
        }

        if (isRoot) {
            if (alpha > bestScore) {
                bestScore = alpha; 
                bestMove = board.moveList[i];
            }
        }
    }
    WriteEntry(board.hashKey, depth, alpha, nodeType);
    return alpha;
}


void SearchPosition(Board &board, int depth) {
    bestScore = -50000;
    bestMove = Move();
    NegaMax(board, depth, -50000, 50000, true); 
    std::cout << "info depth " << depth << " nodes " << nodes << '\n'; 
    std::cout << "bestmove " << squareCoords[bestMove.MoveFrom()]
        << squareCoords[bestMove.MoveTo()] <<  '\n';
    nodes = 0;
}
