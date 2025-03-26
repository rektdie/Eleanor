#include "search.h"
#include "movegen.h"
#include "evaluate.h"
#include <algorithm>
#include <cmath>
#include "tt.h"
#include "stopwatch.h"

namespace SEARCH {

U64 nodes = 0;
bool benchStarted = false;

static inline int timeToSearch = 0;
static inline bool doingNullMove = false;

inline PVLine pvLine;

Stopwatch sw;

static int ScoreMove(Board &board, Move &move, int ply) {
    TTEntry *current = TT.GetRawEntry(board.hashKey);
    if (current->bestMove == move) {
        return 50000;
    }

    if (move.IsCapture()) {
        const int attackerType = board.GetPieceType(move.MoveFrom());
        int targetType = board.GetPieceType(move.MoveTo());

        if (move.GetFlags() == epCapture) {
            targetType = Pawn;
        }

        return moveScoreTable[attackerType][targetType] + 30000;
    } else {
        if (killerMoves[0][ply] == move) {
            return 20000;
        } else if (killerMoves[1][ply] == move) {
            return 18000;
        } else {
            // Max 16384
            return history[board.sideToMove][move.MoveFrom()][move.MoveTo()];
        }
    }

    return 0;
}

static void SortMoves(Board &board, int ply) {
    std::array<int, 218> scores;
    std::array<int, 218> indices;

    // Initialize scores and indices
    for (int i = 0; i < board.currentMoveIndex; i++) {
        scores[i] = ScoreMove(board, board.moveList[i], ply);
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

void ListScores(Board &board, int ply) {
    SortMoves(board, ply);

    for (int i = 0; i < board.currentMoveIndex; i++) {
        Move currentMove = board.moveList[i];

        std::cout << squareCoords[currentMove.MoveFrom()] << squareCoords[currentMove.MoveTo()];
        std::cout << ": " << ScoreMove(board, currentMove, ply) << '\n';
    }
}

static bool IsThreefold(Board &board) {
    for (int i = 0; i < positionIndex; i++) {
        if (positionHistory[i] == board.hashKey) {
            // repetition found
            return true;
        }
    }

    // no repetition
    return false;
}

static bool IsFifty(Board &board) {
    return (board.halfMoves >= 100);
}

static int GetReductions(Board &board, Move &move, int depth, int moveSeen, int ply) {
    int reduction = 0;
    
    // Late Move Reduction
    if (depth >= 3 && moveSeen >= 3 && !move.IsCapture()) {
        double base = 0.77;
        double divisor = 2.36;

        reduction = std::floor(base + std::log(depth) * std::log(moveSeen) / divisor);
    }

    return reduction;
}

static SearchResults Quiescence(Board board, int alpha, int beta, int ply) {
    if (!benchStarted) {
        if (sw.GetElapsedMS() >= timeToSearch) {
            StopSearch();
            return 0;
        }
    }

    int bestScore = HCE::Evaluate(board);

    if (bestScore>= beta) {
        return bestScore;
    }

    if (alpha < bestScore) {
        alpha = bestScore;
    }

    MOVEGEN::GenerateMoves<Noisy>(board);

    SortMoves(board, ply);

    SearchResults results;

    for (int i = 0; i < board.currentMoveIndex; i++) {
        Board copy = board;
        int isLegal = copy.MakeMove(board.moveList[i]);
        if (!isLegal) continue;
        int score = -Quiescence(copy, -beta, -alpha, ply + 1).score;
        positionIndex--;

        if (score >= beta) {
            return score;
        }

        bestScore = std::max(score, bestScore);

        if (score > alpha) {
            alpha = score;
            results.bestMove = board.moveList[i];
        }
    }

    results.score = bestScore;
    if (searchStopped) return 0;
    return results;
}

template <bool isPV>
SearchResults PVS(Board board, int depth, int alpha, int beta, int ply) {
    if (!benchStarted) {
        if (sw.GetElapsedMS() >= timeToSearch) {
            StopSearch();
            return 0;
        }
    }

    pvLine.SetLength(ply);
    if (ply && (IsThreefold(board) || IsFifty(board))) return 0;


    // if NOT PV node then we try to hit the TTable
    if constexpr (!isPV) {
        SearchResults entry = TT.ReadEntry(board.hashKey, depth, alpha, beta);
        if (entry.score != invalidEntry) {
            return entry;
        }
    }

    if (depth <= 0) return Quiescence(board, alpha, beta, ply);

    const int staticEval = HCE::Evaluate(board);

    if (board.InCheck()) {
        depth++;
    } else {
        if (ply) {
            // Reverse Futility Pruning
            int margin = 100 * depth;
            if (staticEval - margin >= beta && depth < 7) {
                return staticEval;
            }

            // Null Move Pruning
            if (!doingNullMove && staticEval >= beta) {
                if (depth >= 3 && !board.InPossibleZug()) {
                    Board copy = board;
                    bool isLegal = copy.MakeMove(Move());
                    // Always legal so we dont check it
    
                    doingNullMove = true;
                    int score = -PVS<false>(copy, depth - 3, -beta, -beta + 1, ply + 1).score;
                    doingNullMove = false;
    
                    if (searchStopped) return 0;
                    if (score >= beta) return score; 
                }
            }
        }
    }

    MOVEGEN::GenerateMoves<All>(board);

    SortMoves(board, ply);

    int score = -inf;
    int nodeType = AllNode;
    SearchResults results(-inf);

    int moveSeen = 0;

    // For all moves
    for (int i = 0; i < board.currentMoveIndex; i++) {
        Move currMove = board.moveList[i];

        // Futility pruning
        // If our static eval is far below alpha, there is only a small chance
        // that a quiet move will help us so we skip them
        int fpMargin = 100 * depth;
        if (!isPV && ply && !board.InCheck() && currMove.IsQuiet()
            && depth <= 5 && staticEval + fpMargin < alpha && results.score > MATE_SCORE) {
            continue;
        }

        Board copy = board;
        bool isLegal = copy.MakeMove(currMove);

        if (!isLegal) continue;

        int reductions = GetReductions(board, currMove, depth, moveSeen, ply);

        // First move (suspected PV node)
        if (!i) {
            // Full search
            score = -PVS<isPV>(copy, depth - 1, -beta, -alpha, ply + 1).score;
        } else if (reductions) {
            // Null-window search with reductions
            score = -PVS<false>(copy, depth - 1 - reductions, -alpha-1, -alpha, ply + 1).score;

            if (score > alpha) {
                // Null-window search now without the reduction
                score = -PVS<false>(copy, depth - 1, -alpha-1, -alpha, ply + 1).score;
            }
        } else {
            // Null-window search
            score = -PVS<false>(copy, depth - 1, -alpha-1, -alpha, ply + 1).score;
        }

        // Check if we need to do full window re-search
        if (i && score > alpha && score < beta) {
            score = -PVS<isPV>(copy, depth - 1, -beta, -alpha, ply + 1).score;
        }

        moveSeen++;
        positionIndex--;

        // Fail high (beta cutoff)
        if (score >= beta) {
            if (!currMove.IsCapture()) {
                killerMoves[1][ply] = killerMoves[0][ply];
                killerMoves[0][ply] = currMove;

                history.Update(board.sideToMove, currMove, depth * depth);
            }

            TT.WriteEntry(board.hashKey, depth, score, CutNode, Move());
            return score;
        }

        results.score = std::max(score, results.score);

        if (score > alpha) {
            nodeType = PV;
            alpha = score;
            results.bestMove = currMove;
            pvLine.SetMove(ply, currMove);
        }
    }

    if (moveSeen == 0) {
        if (board.InCheck()) { // checkmate
            return MATE_SCORE + ply;
        } else { // stalemate
            return 0;
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

    int ply = 0;

    for (int depth = 1; depth <= 99; depth++) {
        timeToSearch = (fullTime / 20) + (inc / 2);
        int softTime = timeToSearch * 0.65;

        SearchResults currentResults = PVS<true>(board, depth, alpha, beta, ply);

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

        if (searchStopped) {
            break;
        } else {
            if (sw.GetElapsedMS() >= softTime) {
                StopSearch();
                break;
            }
        }

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
}
