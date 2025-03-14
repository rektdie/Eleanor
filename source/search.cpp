#include "search.h"
#include "movegen.h"
#include "evaluate.h"
#include <algorithm>
#include "tt.h"
#include "stopwatch.h"

U64 nodes = 0;
bool benchStarted = false;

static inline int ply = 0;
static inline int timeToSearch = 0;
static inline bool doingNullMove = false;

//                          [id][ply]
static inline int killerMoves[2][64];
//                          [stm][from][to]
static inline int historyMoves[2][64][64];

inline PVLine pvLine;

Stopwatch sw;

static int ScoreMove(Board &board, Move &move) {
    TTEntry *current = TT.GetRawEntry(board.hashKey);
    if (current->bestMove == move) {
        return 500000;
    }

    if (move.IsCapture()) {
        const int attackerType = board.GetPieceType(move.MoveFrom());
        int targetType = board.GetPieceType(move.MoveTo());

        if (move.GetFlags() == epCapture) {
            targetType = Pawn;
        }

        return moveScoreTable[attackerType][targetType] + 10000;
    } else {
        if (killerMoves[0][ply] == move) {
            return 490000;
        } else if (killerMoves[1][ply] == move) {
            return 480000;
        } else {
            return historyMoves[board.sideToMove][move.MoveFrom()][move.MoveTo()];
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
    std::stable_sort(indices.begin(), indices.begin() + board.currentMoveIndex,
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
    if (!benchStarted) {
        if (sw.GetElapsedMS() >= timeToSearch) {
            StopSearch();
            return 0;
        }
    }

    int bestScore = Evaluate(board);

    if (bestScore>= beta) {
        return bestScore;
    }

    if (alpha < bestScore) {
        alpha = bestScore;
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
                return score;
            }

            bestScore = std::max(score, bestScore);

            if (score > alpha) {
                alpha = score;
                results.bestMove = board.moveList[i];
            }
        }
    }

    results.score = bestScore;
    if (searchStopped) return 0;
    return results;
}

SearchResults PVS(Board board, int depth, int alpha, int beta) {
    if (!benchStarted) {
        if (sw.GetElapsedMS() >= timeToSearch) {
            StopSearch();
            return 0;
        }
    }

    pvLine.SetLength(ply);

    // if NOT PV node then we try to hit the TTable
    if (beta - alpha == 1) {
        SearchResults entry = TT.ReadEntry(board.hashKey, depth, alpha, beta);
        if (entry.score != invalidEntry) {
            return entry;
        }
    }

    if (depth <= 0) return Quiescence(board, alpha, beta);

    const int staticEval = Evaluate(board);

    if (board.InCheck(board.sideToMove)) {
        depth++;
    } else {
        if (ply) {
            // Reverse Futility Pruning
            int margin = 100 * depth;
            if (staticEval - margin >= beta && depth < 7) {
                return staticEval;
            }

            // Null Move Pruning
            if (!doingNullMove) {
                if (depth >= 3 && !board.InPossibleZug(board.sideToMove)) {
                    Board copy = board;
                    copy.MakeMove(Move());
    
                    doingNullMove = true;
                    ply++;
                    int score = -PVS(copy, depth - 3, -beta, -beta + 1).score;
                    ply--;
                    doingNullMove = false;
    
                    if (searchStopped) return 0;
                    if (score >= beta) return score; 
                }
            }
        }
    }

    GenerateMoves(board, board.sideToMove);

    if (board.currentMoveIndex == 0) {
        if (board.InCheck(board.sideToMove)) { // checkmate
            return -99000 + ply;
        } else { // stalemate
            return 0;
        }
    }

    SortMoves(board);

    int score = -inf;
    int nodeType = AllNode;
    SearchResults results(-inf);

    // For all moves
    for (int i = 0; i < board.currentMoveIndex; i++) {
        Move currMove = board.moveList[i];

        Board copy = board;
        copy.MakeMove(currMove);

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

        // Fail high (beta cutoff)
        if (score >= beta) {
            if (!currMove.IsCapture()) {
                killerMoves[1][ply] = killerMoves[0][ply];
                killerMoves[0][ply] = currMove;
            }

            TT.WriteEntry(board.hashKey, depth, score, CutNode, Move());
            return score;
        }

        results.score = std::max(score, results.score);

        if (score > alpha) {
            if (!currMove.IsCapture()) {
                historyMoves[board.sideToMove][currMove.MoveFrom()][currMove.MoveTo()] += depth * depth;
            }

            nodeType = PV;
            alpha = score;
            results.bestMove = currMove;
            pvLine.SetMove(ply, currMove);
        }
    }

    TT.WriteEntry(board.hashKey, depth, results.score, nodeType, results.bestMove);
    if (searchStopped) return 0;
    return results;
}

// Iterative deepening
static SearchResults ID(Board &board, SearchParams params) {
    sw.Restart();

    int fullTime = board.sideToMove ? params.btime : params.wtime;
    int inc = board.sideToMove ? params.binc : params.winc;

    SearchResults safeResults;
    safeResults.score = -inf;

    int alpha = -inf;
    int beta = inf;

    int delta = 50;

    for (int depth = 1; depth <= 99; depth++) {
        timeToSearch = (fullTime / 20) + (inc / 2);

        SearchResults currentResults = PVS(board, depth, alpha, beta);

        // If we fell outside the window, try again with full width
        if ((currentResults.score <= alpha)
            || (currentResults.score >= beta)) {
            delta *= 2;

            alpha -= delta;
            beta += delta;

            depth--;

            if (searchStopped) break;
            continue;
        }

        alpha = currentResults.score - delta;
        beta = currentResults.score + delta;
        delta = 50;

        if (currentResults.bestMove) {
            safeResults = currentResults;
        }

        if (searchStopped) break;

        std::cout << "info ";
        std::cout << "depth " << depth;
        std::cout << " score cp " << safeResults.score;
        std::cout << " nodes " << nodes << " nps " << int(nodes/sw.GetElapsedSec());
        std::cout << " hashfull " << TT.GetUsedPercentage();
        std::cout << " pv ";
        pvLine.Print(0);
        std::cout << std::endl;
    }

    return safeResults;
}

void SearchPosition(Board &board, SearchParams params) {
    searchStopped = false;
    nodes = 0;
    pvLine.Clear();

    SearchResults results = ID(board, params);

    std::cout << "bestmove ";
    results.bestMove.PrintMove();
    std::cout << std::endl;
}

void StopSearch() {
    searchStopped = true;
}
