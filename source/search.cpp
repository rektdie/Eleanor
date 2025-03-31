#include "search.h"
#include "movegen.h"
#include "evaluate.h"
#include <algorithm>
#include <cmath>
#include "tt.h"
#include "stopwatch.h"
#include "datagen.h"
#include "benchmark.h"

namespace SEARCH {

U64 nodes = 0;
bool benchStarted = false;

static inline int timeToSearch = 0;
static inline bool doingNullMove = false;

inline PVLine pvLine;

Stopwatch sw;

void InitLMRTable() {
    for (int depth = 0; depth <= MAX_DEPTH; depth++) {
        for (int move = 0; move < MAX_MOVES; move++) {
            if (depth == 0 || move == 0) {
                lmrTable[depth][move] = 0;
            } else {
                double base = 0.77;
                double divisor = 2.36;

                lmrTable[depth][move] = std::floor(base + std::log(depth) * std::log(move) / divisor);
            }
        }
    }
}

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
    std::array<int, MAX_MOVES> scores;
    std::array<int, MAX_MOVES> indices;

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
    std::array<Move, MAX_MOVES> sortedMoves;
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

static bool IsInsuffMat(Board &board) {
    return (board.occupied.PopCount() <= 3 
        && !(board.pieces[Pawn] | board.pieces[Queen] | board.pieces[Rook]));
}

static int GetReductions(Board &board, Move &move, int depth, int moveSeen, int ply) {
    int reduction = 0;
    
    // Late Move Reduction
    if (depth >= 3 && moveSeen >= 3 && !move.IsCapture()) {
        reduction = lmrTable[depth][moveSeen];
    }

    return reduction;
}

static SearchResults Quiescence(Board board, int alpha, int beta, int ply) {
    if (!benchStarted && nodes % 1024 == 0) {
        if (sw.GetElapsedMS() >= timeToSearch) {
            StopSearch();
            return 0;
        }
    }

    int bestScore = HCE::Evaluate(board);

    TTEntry *entry = TT.GetRawEntry(board.hashKey);
    if (entry->hashKey == board.hashKey) {
        bestScore = entry->score;
    }


    if (bestScore >= beta) {
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

template <bool isPV, searchMode mode>
SearchResults PVS(Board board, int depth, int alpha, int beta, int ply) {
    if constexpr (mode != bench) {
        if (nodes % 1024 == 0) {
            if constexpr (mode == normal) {
                if (sw.GetElapsedMS() >= timeToSearch) {
                    StopSearch();
                    return 0;
                }
            } else {
                if (nodes >= DATAGEN::HARD_NODES) {
                    StopSearch();
                    return 0;
                }
            }
        }
    }

    pvLine.SetLength(ply);
    if (ply && (IsThreefold(board) || IsFifty(board) || IsInsuffMat(board))) return 0;


    // if NOT PV node then we try to hit the TTable
    if constexpr (!isPV) {
        SearchResults entry = TT.ReadEntry(board.hashKey, depth, alpha, beta);
        if (entry.score != invalidEntry) {
            return entry;
        }
    }

    if (depth <= 0) return Quiescence(board, alpha, beta, ply);

    const int staticEval = HCE::Evaluate(board);

    if (!board.InCheck()) {
        if (ply) {
            // Reverse Futility Pruning
            int margin = 100 * depth;
            if (staticEval - margin >= beta && depth < 7) {
                return (beta + (staticEval - beta) / 3);
            }

            // Null Move Pruning
            if (!doingNullMove && staticEval >= beta) {
                if (depth >= 3 && !board.InPossibleZug()) {
                    Board copy = board;
                    bool isLegal = copy.MakeMove(Move());
                    // Always legal so we dont check it
    
                    doingNullMove = true;
                    int score = -PVS<false, mode>(copy, depth - 3, -beta, -beta + 1, ply + 1).score;
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
    std::array<Move, MAX_MOVES> seenQuiets;
    int seenQuietsCount = 0;

    // For all moves
    for (int i = 0; i < board.currentMoveIndex; i++) {
        Move currMove = board.moveList[i];

        // Futility pruning
        // If our static eval is far below alpha, there is only a small chance
        // that a quiet move will help us so we skip them
        int fpMargin = 100 * depth;
        if (!isPV && ply && currMove.IsQuiet()
            && depth <= 5 && staticEval + fpMargin < alpha && results.score > MATE_SCORE) {
            continue;
        }

        Board copy = board;
        bool isLegal = copy.MakeMove(currMove);

        if (!isLegal) continue;

        int reductions = GetReductions(board, currMove, depth, moveSeen, ply);

        int newDepth = depth + copy.InCheck() - 1;

        // First move (suspected PV node)
        if (!moveSeen) {
            // Full search
            score = -PVS<isPV, mode>(copy, newDepth, -beta, -alpha, ply + 1).score;
        } else if (reductions) {
            // Null-window search with reductions
            score = -PVS<false, mode>(copy, newDepth - reductions, -alpha-1, -alpha, ply + 1).score;

            if (score > alpha) {
                // Null-window search now without the reduction
                score = -PVS<false, mode>(copy, newDepth, -alpha-1, -alpha, ply + 1).score;
            }
        } else {
            // Null-window search
            score = -PVS<false, mode>(copy, newDepth, -alpha-1, -alpha, ply + 1).score;
        }

        // Check if we need to do full window re-search
        if (moveSeen && score > alpha && score < beta) {
            score = -PVS<isPV, mode>(copy, newDepth, -beta, -alpha, ply + 1).score;
        }

        if (searchStopped) return 0;

        if (currMove.IsQuiet()) {
            seenQuiets[seenQuietsCount] = currMove;
            seenQuietsCount++;
        }

        moveSeen++;
        positionIndex--;

        // Fail high (beta cutoff)
        if (score >= beta) {
            if (!currMove.IsCapture()) {
                killerMoves[1][ply] = killerMoves[0][ply];
                killerMoves[0][ply] = currMove;

                int bonus = depth * depth;

                history.Update(board.sideToMove, currMove, bonus);

                for (int moveIndex = 0; moveIndex < seenQuietsCount - 1; moveIndex++) {
                    history.Update(board.sideToMove, seenQuiets[moveIndex], -bonus);
                }
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

    if (searchStopped) return 0;
    TT.WriteEntry(board.hashKey, depth, results.score, nodeType, results.bestMove);
    return results;
}

// Iterative deepening
template <searchMode mode>
static SearchResults ID(Board &board, SearchParams params) {
    int fullTime = board.sideToMove ? params.btime : params.wtime;
    int inc = board.sideToMove ? params.binc : params.winc;

    SearchResults safeResults;
    safeResults.score = -inf;

    int toDepth = MAX_DEPTH;

    if constexpr (mode == bench) {
        toDepth = BENCH_DEPTH;
    }

    int alpha = -inf;
    int beta = inf;

    int delta = 50;

    int ply = 0;

    int elapsed = 0;

    if constexpr (mode == normal) {
        sw.Restart();
    }

    for (int depth = 1; depth <= toDepth; depth++) {
        timeToSearch = std::max((fullTime / 20) + (inc / 2), 4);
        int softTime = timeToSearch * 0.65;

        SearchResults currentResults = PVS<true, mode>(board, depth, alpha, beta, ply);
        elapsed = sw.GetElapsedMS();

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

        if (searchStopped) {
            break;
        } else {
            if (currentResults.bestMove) {
                safeResults = currentResults;
            }

            if constexpr (mode == normal) {
                std::cout << "info ";
                std::cout << "depth " << depth;
                std::cout << " time " << elapsed;
                std::cout << " score cp " << safeResults.score;
                std::cout << " nodes " << nodes << " nps " << int(nodes/sw.GetElapsedSec());
                std::cout << " hashfull " << TT.GetUsedPercentage();
                std::cout << " pv ";
                pvLine.Print(0);
                std::cout << std::endl;

                if (sw.GetElapsedMS() >= softTime) {
                    StopSearch();
                    break;
                }
            } else if constexpr (mode == datagen) {
                if (nodes >= DATAGEN::SOFT_NODES) {
                    StopSearch();
                    break;
                }
            }
        }
    }

    return safeResults;
}

template <searchMode mode>
void SearchPosition(Board &board, SearchParams params) {
    searchStopped = false;
    if constexpr (mode != bench) {
        nodes = 0;
    }
    pvLine.Clear();

    SearchResults results = ID<mode>(board, params);

    if constexpr (mode != normal) return;

    std::cout << "bestmove ";
    results.bestMove.PrintMove();
    std::cout << std::endl;
}

void StopSearch() {
    searchStopped = true;
}

template SearchResults PVS<true, searchMode::bench>(Board, int, int, int, int);
template SearchResults PVS<false, searchMode::bench>(Board, int, int, int, int);
template SearchResults PVS<true, searchMode::normal>(Board, int, int, int, int);
template SearchResults PVS<false, searchMode::normal>(Board, int, int, int, int);
template SearchResults PVS<true, searchMode::datagen>(Board, int, int, int, int);
template SearchResults PVS<false, searchMode::datagen>(Board, int, int, int, int);

template void SearchPosition<normal>(Board &board, SearchParams params);
template void SearchPosition<bench>(Board &board, SearchParams params);
template void SearchPosition<datagen>(Board &board, SearchParams params);

}
