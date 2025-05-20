#include "search.h"
#include "movegen.h"
#include <algorithm>
#include <cmath>
#include "datagen.h"
#include "benchmark.h"

namespace SEARCH {

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

static int ScoreMove(Board &board, Move &move, int ply, SearchContext& ctx) {
    TTEntry *current = ctx.TT.GetRawEntry(board.hashKey);
    if (current->hashKey == board.hashKey && current->bestMove == move) {
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
        if (ctx.killerMoves[0][ply] == move) {
            return 20000;
        } else if (ctx.killerMoves[1][ply] == move) {
            return 18000;
        } else {
            // Max 16384
            return ctx.history[board.sideToMove][move.MoveFrom()][move.MoveTo()];
        }
    }

    return 0;
}

static void SortMoves(Board &board, int ply, SearchContext& ctx) {
    if (board.currentMoveIndex <= 1) {
        return;
    }
    
    int scores[MAX_MOVES];
    
    for (int i = 0; i < board.currentMoveIndex; i++) {
        scores[i] = ScoreMove(board, board.moveList[i], ply, ctx);
    }
    
    for (int i = 1; i < board.currentMoveIndex; i++) {
        Move tempMove = board.moveList[i];
        int tempScore = scores[i];
        int j = i - 1;
        
        while (j >= 0 && scores[j] < tempScore) {
            board.moveList[j + 1] = board.moveList[j];
            scores[j + 1] = scores[j];
            j--;
        }
        
        board.moveList[j + 1] = tempMove;
        scores[j + 1] = tempScore;
    }
}

void ListScores(Board &board, int ply, SearchContext& ctx) {
    SortMoves(board, ply, ctx);

    for (int i = 0; i < board.currentMoveIndex; i++) {
        Move currentMove = board.moveList[i];

        std::cout << squareCoords[currentMove.MoveFrom()] << squareCoords[currentMove.MoveTo()];
        std::cout << ": " << ScoreMove(board, currentMove, ply, ctx) << '\n';
    }
}

static bool IsThreefold(Board &board, SearchContext& ctx) {
    for (int i = 0; i < board.positionIndex; i++) {
        if (ctx.positionHistory[i] == board.hashKey) {
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

bool IsDraw(Board &board, SearchContext& ctx) {
    return IsFifty(board) || IsInsuffMat(board) || IsThreefold(board, ctx);
}

static int GetReductions(Board &board, Move &move, int depth, int moveSeen, int ply) {
    int reduction = 0;
    
    // Late Move Reduction
    if (depth >= 3 && moveSeen >= 3 && !move.IsCapture()) {
        reduction = lmrTable[depth][moveSeen];
    }

    return reduction;
}

template <searchMode mode>
static SearchResults Quiescence(Board& board, int alpha, int beta, int ply, SearchContext& ctx) {
    if constexpr (mode != nodesMode)  {
        if constexpr (mode != bench) {
            if (ctx.nodes % 1024 == 0) {
                if (ctx.sw.GetElapsedMS() >= ctx.timeToSearch) {
                    ctx.searchStopped = true;
                    return 0;
                }
            }
        }
    } else if constexpr (mode == nodesMode) {
        if (ctx.nodes > ctx.nodesToGo) {
            ctx.searchStopped = true;
            return 0;
        }
    } else if constexpr (mode == datagen) {
        if (ctx.nodes > DATAGEN::HARD_NODES) {
            ctx.searchStopped = true;
            return 0;
        }
    }

    if (ply > ctx.seldepth)
        ctx.seldepth = ply;
    
    int bestScore = NNUE::net.Evaluate(board);
    ctx.ss[ply].eval = bestScore;

    TTEntry *entry = ctx.TT.GetRawEntry(board.hashKey);
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

    SortMoves(board, ply, ctx);

    SearchResults results;

    for (int i = 0; i < board.currentMoveIndex; i++) {
        Board copy = board;
        int isLegal = copy.MakeMove(board.moveList[i]);
        if (!isLegal) continue;
        if (copy.positionIndex >= ctx.positionHistory.size()) {
            ctx.positionHistory.resize(copy.positionIndex + 100);
        }
        ctx.positionHistory[copy.positionIndex] = copy.hashKey;
        ctx.nodes++;
        int score = -Quiescence<mode>(copy, -beta, -alpha, ply + 1, ctx).score;

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
    if (ctx.searchStopped) return 0;
    return results;
}

template <bool isPV, searchMode mode>
SearchResults PVS(Board& board, int depth, int alpha, int beta, int ply, SearchContext& ctx) {
    if constexpr (mode != bench && mode != nodesMode) {
        if (ctx.nodes % 1024 == 0) {
            if constexpr (mode == normal) {
                if (ctx.sw.GetElapsedMS() >= ctx.timeToSearch) {
                    ctx.searchStopped = true;
                    return 0;
                }
            }
        }
    } else if constexpr (mode == nodesMode) {
        if (ctx.nodes > ctx.nodesToGo) {
            ctx.searchStopped = true;
            return 0;
        }
    } else if constexpr (mode == datagen) {
        if (ctx.nodes > DATAGEN::HARD_NODES) {
            ctx.searchStopped = true;
            return 0;
        }
    }

    if (ply > ctx.seldepth)
        ctx.seldepth = ply;


    ctx.pvLine.SetLength(ply);
    if (ply && (IsDraw(board, ctx))) return 0;


    // if NOT PV node then we try to hit the TTable
    if constexpr (!isPV) {
        SearchResults entry = ctx.TT.ReadEntry(board.hashKey, depth, alpha, beta);
        if (entry.score != invalidEntry) {
            return entry;
        }
    }

    if (depth <= 0) return Quiescence<mode>(board, alpha, beta, ply, ctx);

    const int staticEval = NNUE::net.Evaluate(board);
    ctx.ss[ply].eval = staticEval;

    const bool improving = [&]
    {
        if (board.InCheck())
            return false;
        if (ply > 1 && ctx.ss[ply - 2].eval != ScoreNone)
            return staticEval > ctx.ss[ply - 2].eval;
        if (ply > 3 && ctx.ss[ply - 4].eval != ScoreNone)
            return staticEval > ctx.ss[ply - 4].eval;

        return true;
    }();

    if (!board.InCheck()) {
        if (ply) {
            // Reverse Futility Pruning
            int margin = 100 * (depth - improving);
            if (staticEval - margin >= beta && depth < 7) {
                return (beta + (staticEval - beta) / 3);
            }

            // Null Move Pruning
            if (!ctx.doingNullMove && staticEval >= beta) {
                if (depth >= 3 && !board.InPossibleZug()) {
                    Board copy = board;
                    bool isLegal = copy.MakeMove(Move());
                    // Always legal so we dont check it
    
                    ctx.doingNullMove = true;
                    int score = -PVS<false, mode>(copy, depth - 3, -beta, -beta + 1, ply + 1, ctx).score;
                    ctx.doingNullMove = false;
    
                    if (ctx.searchStopped) return 0;
                    if (score >= beta) return score; 
                }
            }
        }
    }

    MOVEGEN::GenerateMoves<All>(board);

    SortMoves(board, ply, ctx);

    int score = -inf;
    int nodeType = AllNode;
    SearchResults results(-inf);

    int moveSeen = 0;
    std::array<Move, MAX_MOVES> seenQuiets;
    int seenQuietsCount = 0;

    // For all moves
    for (int i = 0; i < board.currentMoveIndex; i++) {
        Move currMove = board.moveList[i];

        // Late move pruning
        // If we are near a leaf node we prune moves
        // that are late in the list
        if (!isPV && !board.InCheck() && currMove.IsQuiet() && results.score > (-MATE_SCORE + MAX_PLY)) {
            int lmpBase = 7;

            int lmpThreshold = lmpBase + 4 * depth;

            if (moveSeen >= lmpThreshold) {
                continue;
            }
        }

        // Futility pruning
        // If our static eval is far below alpha, there is only a small chance
        // that a quiet move will help us so we skip them
        int fpMargin = 100 * depth;
        if (!isPV && ply && currMove.IsQuiet()
            && depth <= 5 && staticEval + fpMargin < alpha && results.score > -MATE_SCORE) {
            continue;
        }

        Board copy = board;
        bool isLegal = copy.MakeMove(currMove);

        if (!isLegal) continue;
        if (copy.positionIndex >= ctx.positionHistory.size()) {
            ctx.positionHistory.resize(copy.positionIndex + 100);
        }
        ctx.positionHistory[copy.positionIndex] = copy.hashKey;
        ctx.nodes++;

        int reductions = GetReductions(board, currMove, depth, moveSeen, ply);

        int newDepth = depth + copy.InCheck() - 1;

        // First move (suspected PV node)
        if (!moveSeen) {
            // Full search
            score = -PVS<isPV, mode>(copy, newDepth, -beta, -alpha, ply + 1, ctx).score;
        } else if (reductions) {
            // Null-window search with reductions
            score = -PVS<false, mode>(copy, newDepth - reductions, -alpha-1, -alpha, ply + 1, ctx).score;

            if (score > alpha) {
                // Null-window search now without the reduction
                score = -PVS<false, mode>(copy, newDepth, -alpha-1, -alpha, ply + 1, ctx).score;
            }
        } else {
            // Null-window search
            score = -PVS<false, mode>(copy, newDepth, -alpha-1, -alpha, ply + 1, ctx).score;
        }

        // Check if we need to do full window re-search
        if (moveSeen && score > alpha && score < beta) {
            score = -PVS<isPV, mode>(copy, newDepth, -beta, -alpha, ply + 1, ctx).score;
        }

        moveSeen++;

        if (ctx.searchStopped) return 0;

        if (currMove != 0 && currMove.IsQuiet()) {
            seenQuiets[seenQuietsCount] = currMove;
            seenQuietsCount++;
        }

        // Fail high (beta cutoff)
        if (score >= beta) {
            if (!currMove.IsCapture()) {
                ctx.killerMoves[1][ply] = ctx.killerMoves[0][ply];
                ctx.killerMoves[0][ply] = currMove;

                int bonus = depth * depth;

                ctx.history.Update(board.sideToMove, currMove, bonus);

                for (int moveIndex = 0; moveIndex < seenQuietsCount - 1; moveIndex++) {
                    ctx.history.Update(board.sideToMove, seenQuiets[moveIndex], -bonus);
                }
            }

            ctx.TT.WriteEntry(board.hashKey, depth, score, CutNode, Move());
            return score;
        }

        results.score = std::max(score, results.score);

        if (score > alpha) {
            nodeType = PV;
            alpha = score;
            results.bestMove = currMove;
            ctx.pvLine.SetMove(ply, currMove);
        }
    }

    if (moveSeen == 0) {
        if (board.InCheck()) { // checkmate
            return -MATE_SCORE + ply;
        } else { // stalemate
            return 0;
        }
    }

    if (ctx.searchStopped) return 0;
    ctx.TT.WriteEntry(board.hashKey, depth, results.score, nodeType, results.bestMove);
    return results;
}

// Iterative deepening
template <searchMode mode>
static SearchResults ID(Board &board, SearchParams params, SearchContext& ctx) {
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

    ctx.sw.Restart();

    for (int depth = 1; depth <= toDepth; depth++) {
        ctx.timeToSearch = std::max((fullTime / 20) + (inc / 2), 4);
        int softTime = ctx.timeToSearch * 0.65;

        SearchResults currentResults = PVS<true, mode>(board, depth, alpha, beta, ply, ctx);
        elapsed = ctx.sw.GetElapsedMS();

        // If we fell outside the window, try again with full width
        if ((currentResults.score <= alpha)
            || (currentResults.score >= beta)) {
            delta *= 2;

            alpha -= delta;
            beta += delta;

            depth--;

            if (ctx.searchStopped) break;
            continue;
        }

        alpha = currentResults.score - delta;
        beta = currentResults.score + delta;
        delta = 50;

        if (ctx.searchStopped) {
            break;
        } else {
            if (currentResults.bestMove) {
                safeResults = currentResults;
            }

            if constexpr (mode == normal || mode == nodesMode) {
                std::cout << "info ";
                std::cout << "depth " << depth;
                std::cout << " seldepth " << ctx.seldepth;
                std::cout << " time " << elapsed;
                std::cout << " score cp " << UTILS::ConvertToWhiteRelative(board, safeResults.score);
                std::cout << " nodes " << ctx.nodes << " nps " << int(ctx.nodes/ctx.sw.GetElapsedSec());
                std::cout << " hashfull " << ctx.TT.GetUsedPercentage();
                std::cout << " pv ";
                ctx.pvLine.Print(0);
                std::cout << std::endl;

                if constexpr (mode == normal) {
                    if (ctx.sw.GetElapsedMS() >= softTime) {
                        ctx.searchStopped = true;
                        break;
                    }
                }
            } else if constexpr (mode == datagen) {
                if (ctx.nodes >= DATAGEN::SOFT_NODES) {
                    ctx.searchStopped = true;
                    break;
                }
            }
        }
    }

    return safeResults;
}

template <searchMode mode>
SearchResults SearchPosition(Board &board, SearchParams params, SearchContext& ctx) {
    ctx.searchStopped = false;
    ctx.seldepth = 0;
    if constexpr (mode != bench) {
        ctx.nodes = 0;

        if constexpr (mode == nodesMode) {
            ctx.nodesToGo = params.nodes;
        }
    }
    ctx.pvLine.Clear();

    SearchResults results = ID<mode>(board, params, ctx);

    if constexpr (mode != normal) return results;

    std::cout << "bestmove ";
    results.bestMove.PrintMove();
    std::cout << std::endl;

    return results;
}

template SearchResults PVS<true, searchMode::bench>(Board&, int, int, int, int, SearchContext& ctx);
template SearchResults PVS<false, searchMode::bench>(Board&, int, int, int, int, SearchContext& ctx);
template SearchResults PVS<true, searchMode::normal>(Board&, int, int, int, int, SearchContext& ctx);
template SearchResults PVS<false, searchMode::normal>(Board&, int, int, int, int, SearchContext& ctx);
template SearchResults PVS<true, searchMode::datagen>(Board&, int, int, int, int, SearchContext& ctx);
template SearchResults PVS<false, searchMode::datagen>(Board&, int, int, int, int, SearchContext& ctx);

template SearchResults SearchPosition<normal>(Board &board, SearchParams params, SearchContext& ctx);
template SearchResults SearchPosition<bench>(Board &board, SearchParams params, SearchContext& ctx);
template SearchResults SearchPosition<datagen>(Board &board, SearchParams params, SearchContext& ctx);
template SearchResults SearchPosition<nodesMode>(Board &board, SearchParams params, SearchContext& ctx);

}
