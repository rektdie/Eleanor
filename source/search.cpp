#include "search.h"
#include "movegen.h"
#include <algorithm>
#include <cmath>
#include "datagen.h"
#include "benchmark.h"
#include "types.h"

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

int16_t ContHistory::GetOnePly(Board& board, Move& move, SearchContext* ctx, int ply) {
    int prevType = ctx->ss[ply-1].pieceType;
    int prevTo = ctx->ss[ply-1].moveTo;
    int pieceType = board.GetPieceType(move.MoveFrom());
    int to = move.MoveTo();
    bool otherColor = ctx->ss[ply-1].side;

    return ctx->conthist[otherColor][prevType][prevTo][board.sideToMove][pieceType][to];
}

int16_t ContHistory::GetTwoPly(Board& board, Move& move, SearchContext* ctx, int ply) {
    int prevType = ctx->ss[ply-2].pieceType;
    int prevTo = ctx->ss[ply-2].moveTo;
    int pieceType = board.GetPieceType(move.MoveFrom());
    int to = move.MoveTo();
    bool otherColor = ctx->ss[ply-2].side;

    return ctx->conthist[otherColor][prevType][prevTo][board.sideToMove][pieceType][to];
}

static int AdjustEval(Board &board, SearchContext* ctx, int eval) {
    int pawnHist = ctx->corrhist.GetPawnHist(board);

    int mateFound = MATE_SCORE - MAX_PLY;

    return std::clamp(eval + pawnHist / CORRHIST_GRAIN, -mateFound + 1, mateFound - 1);
}

static int ScoreMove(Board &board, Move &move, int ply, SearchContext* ctx) {
    TTEntry current = ctx->TT.GetRawEntry(board.hashKey);
    if (current.hashKey == board.hashKey && current.bestMove == move) {
        return 100000;
    }

    if (move.IsCapture()) {
        const int attackerType = board.GetPieceType(move.MoveFrom());
        int targetType = board.GetPieceType(move.MoveTo());

        if (move.GetFlags() == epCapture) {
            targetType = Pawn;
        }

        return 50000 * ((SEE(board, move, -100))) + (100 * targetType - attackerType + 105);
    } else {
        if (ctx->killerMoves[0][ply] == move) {
            return 41000;
        } else if (ctx->killerMoves[1][ply] == move) {
            return 40000;
        } else {
            int historyScore = ctx->history[board.sideToMove][move.MoveFrom()][move.MoveTo()];
            int conthistScore = 0;

            if (ply > 0) {
                conthistScore = ctx->conthist.GetOnePly(board, move, ctx, ply);
                /*
                if (ply > 1) {
                    conthistScore += ctx->conthist.GetTwoPly(board, move, ctx, ply);
                }
                */
            }

            return 20000 + historyScore + conthistScore;
        }
    }

    return 0;
}

static void SortMoves(Board &board, int ply, SearchContext* ctx) {
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

void ListScores(Board &board, int ply, SearchContext* ctx) {
    SortMoves(board, ply, ctx);

    for (int i = 0; i < board.currentMoveIndex; i++) {
        Move currentMove = board.moveList[i];

        std::cout << squareCoords[currentMove.MoveFrom()] << squareCoords[currentMove.MoveTo()];
        std::cout << ": " << ScoreMove(board, currentMove, ply, ctx) << '\n';
    }
}

static bool IsThreefold(Board &board, SearchContext* ctx) {
    for (int i = 0; i < board.positionIndex; i++) {
        if (ctx->positionHistory[i] == board.hashKey) {
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

bool IsDraw(Board &board, SearchContext* ctx) {
    return IsFifty(board) || IsInsuffMat(board) || IsThreefold(board, ctx);
}

template <bool isPV>
static int GetReductions(Board &board, Move &move, int depth, int moveSeen, int ply, bool cutnode, SearchContext* ctx) {
    int reduction = 0;

    // Late Move Reduction
    if (depth >= 3 && moveSeen >= 2 + (2 * isPV) && !move.IsCapture()) {
        reduction = lmrTable[depth][moveSeen];

        if (cutnode)
            reduction += 2;

        if constexpr (isPV)
            reduction--;

        // History LMR
        int historyReduction = ctx->history[board.sideToMove][move.MoveFrom()][move.MoveTo()];

        if (ply > 0)
            historyReduction += ctx->conthist.GetOnePly(board, move, ctx, ply);

        historyReduction /= 8192;
        reduction -= historyReduction;
    }

    return std::clamp(reduction, -1, depth - 1);
}

bool SEE(Board& board, Move& move, int threshold) {
    int from = move.MoveFrom();
    int to = move.MoveTo();
    int flags = move.GetFlags();

    int nextVictim = board.GetPieceType(from);

    // Next victim is moved piece or promo piece
    if (move.IsPromo()) {
        if (move.IsCapture()) {
            nextVictim = move.GetFlags() - 9;
        } else {
            nextVictim = move.GetFlags() - 5;
        }
    }

    int balance = MoveEstimatedValue(board, move) - threshold;

    if (balance < 0) return 0;

    balance -= SEEPieceValues[nextVictim];

    if (balance >= 0) return 1;

    Bitboard bishops = board.pieces[Bishop] | board.pieces[Queen];
    Bitboard rooks = board.pieces[Rook] | board.pieces[Queen];

    Bitboard occupied = board.occupied;
    occupied.PopBit(from);
    occupied.SetBit(to);

    if (flags == epCapture)
        occupied.SetBit(board.enPassantTarget);

    Bitboard attackers = board.AttacksTo(to, occupied) & occupied;

    bool color = !board.sideToMove;

    while (true) {
        Bitboard myAttackers = attackers & board.colors[color];
        if (!myAttackers) break;

        for (nextVictim = Pawn; nextVictim <= Queen; nextVictim++) {
            if (myAttackers & board.pieces[nextVictim])
                break;
        }

        occupied.PopBit(Bitboard(myAttackers & board.pieces[nextVictim]).getLS1BIndex());

        if (nextVictim == Pawn || nextVictim == Bishop || nextVictim == Queen)
            attackers |= MOVEGEN::getBishopAttack(to, occupied) & bishops;

        if (nextVictim == Rook || nextVictim == Queen)
            attackers |= MOVEGEN::getRookAttack(to, occupied) & rooks;

        attackers &= occupied;

        color = !color;

        balance = -balance - 1 - SEEPieceValues[nextVictim];

        if (balance >= 0) {
            if (nextVictim == King && (attackers & board.colors[color]))
                color = !color;

            break;
        }
    }

    return board.sideToMove != color;
}

template <searchMode mode>
static SearchResults Quiescence(Board& board, int alpha, int beta, int ply, SearchContext* ctx) {
    if constexpr (mode != nodesMode)  {
        if constexpr (mode != bench) {
            if (ctx->nodes % 1024 == 0) {
                if (ctx->sw.GetElapsedMS() >= ctx->timeToSearch) {
                    ctx->searchStopped = true;
                    return 0;
                }
            }
        }
    } else if constexpr (mode == nodesMode) {
        if (ctx->nodes > ctx->nodesToGo) {
            ctx->searchStopped = true;
            return 0;
        }
    } else if constexpr (mode == datagen) {
        if (ctx->nodes > DATAGEN::HARD_NODES) {
            ctx->searchStopped = true;
            return 0;
        }
    }

    if (ply > ctx->seldepth)
        ctx->seldepth = ply;

    int bestScore = AdjustEval(board, ctx, NNUE::net.Evaluate(board));
    ctx->ss[ply].eval = bestScore;

    if (!ctx->excluded) {
        TTEntry entry = ctx->TT.GetRawEntry(board.hashKey);
        if (entry.hashKey == board.hashKey) {
            bestScore = entry.score;
        }
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

        ctx->ss[ply].pieceType = board.GetPieceType(board.moveList[i].MoveFrom());
        ctx->ss[ply].moveTo = board.moveList[i].MoveTo();
        ctx->ss[ply].side = board.sideToMove;

        if (!SEE(board, board.moveList[i], 0))
            continue;

        if (copy.positionIndex >= ctx->positionHistory.size()) {
            ctx->positionHistory.resize(copy.positionIndex + 100);
        }
        ctx->positionHistory[copy.positionIndex] = copy.hashKey;
        ctx->nodes++;
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
    if (ctx->searchStopped) return 0;
    return results;
}

template <bool isPV, searchMode mode>
SearchResults PVS(Board& board, int depth, int alpha, int beta, int ply, SearchContext* ctx, bool cutnode) {
    if constexpr (mode != bench && mode != nodesMode) {
        if (ctx->nodes % 1024 == 0) {
            if constexpr (mode == normal) {
                if (ctx->sw.GetElapsedMS() >= ctx->timeToSearch) {
                    ctx->searchStopped = true;
                    return 0;
                }
            }
        }
    } else if constexpr (mode == nodesMode) {
        if (ctx->nodes > ctx->nodesToGo) {
            ctx->searchStopped = true;
            return 0;
        }
    } else if constexpr (mode == datagen) {
        if (ctx->nodes > DATAGEN::HARD_NODES) {
            ctx->searchStopped = true;
            return 0;
        }
    }

    if (ply > ctx->seldepth)
        ctx->seldepth = ply;


    ctx->pvLine.SetLength(ply);
    if (ply && (IsDraw(board, ctx))) return 0;


    TTEntry entry;
    if (!ctx->excluded)
        entry = ctx->TT.GetRawEntry(board.hashKey);

    const bool ttHit = entry.hashKey == board.hashKey;

    if constexpr (!isPV) {
        if (ttHit) {
            if (entry.depth >= depth &&
                ((entry.nodeType == PV) ||
                (entry.nodeType == AllNode && entry.score <= alpha) ||
                (entry.nodeType == CutNode && entry.score >= beta))) {

                return SearchResults(entry.score, entry.bestMove);
            }
        }
    }

    if (depth <= 0) return Quiescence<mode>(board, alpha, beta, ply, ctx);

    const int staticEval = AdjustEval(board, ctx, NNUE::net.Evaluate(board));
    ctx->ss[ply].eval = staticEval;

    const bool improving = [&]
    {
        if (board.InCheck())
            return false;
        if (ply > 1 && ctx->ss[ply - 2].eval != ScoreNone)
            return staticEval > ctx->ss[ply - 2].eval;
        if (ply > 3 && ctx->ss[ply - 4].eval != ScoreNone)
            return staticEval > ctx->ss[ply - 4].eval;

        return true;
    }();

    if (!board.InCheck() && !ctx->excluded) {
        if (ply) {
            // Reverse Futility Pruning
            int margin = 100 * (depth - improving);
            if (!ttHit && staticEval - margin >= beta && depth < 7) {
                return (beta + (staticEval - beta) / 3);
            }

            // Null Move Pruning
            if (!ctx->doingNullMove && staticEval >= beta) {
                if (depth > 1 && !board.InPossibleZug()) {
                    Board copy = board;
                    copy.MakeMove(Move());

                    const int reduction = 4 + improving + depth / 3;

                    ctx->doingNullMove = true;
                    int score = -PVS<false, mode>(copy, depth - reduction, -beta, -beta + 1, ply + 1, ctx, !cutnode).score;
                    ctx->doingNullMove = false;

                    if (ctx->searchStopped) return 0;
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

        if (ctx->excluded == currMove)
            continue;

        bool notMated = results.score > (-MATE_SCORE + MAX_PLY);

        // Late move pruning
        // If we are near a leaf node we prune moves
        // that are late in the list
        if (!isPV && !board.InCheck() && currMove.IsQuiet() && notMated) {
            int lmpBase = 7;


            int lmpThreshold = lmpBase + 4 * depth;

            if (moveSeen >= lmpThreshold) {
                continue;
            }
        }

        // Futility pruning
        // If our static eval is far below alpha, there is only a small chance
        // that a quiet move will help us so we skip them
        int historyScore = ctx->history[board.sideToMove][currMove.MoveFrom()][currMove.MoveTo()];
        
        if (ply > 0) {
            historyScore += ctx->conthist.GetOnePly(board, currMove, ctx, ply);
            
            if (ply > 1)
                historyScore += ctx->conthist.GetTwoPly(board, currMove, ctx, ply);
        }
        
        int fpMargin = 100 * depth + historyScore / 32;

        if (!isPV && ply && currMove.IsQuiet()
                && depth <= 5 && staticEval + fpMargin < alpha && notMated) {
            continue;
        }

        Board copy = board;
        bool isLegal = copy.MakeMove(currMove);

        if (!isLegal) continue;

        ctx->ss[ply].pieceType = board.GetPieceType(currMove.MoveFrom());
        ctx->ss[ply].moveTo = currMove.MoveTo();
        ctx->ss[ply].side = board.sideToMove;

        if (copy.positionIndex >= ctx->positionHistory.size()) {
            ctx->positionHistory.resize(copy.positionIndex + 100);
        }
        ctx->positionHistory[copy.positionIndex] = copy.hashKey;
        ctx->nodes++;

        int extension = 0;

        if (ply
            && depth >= 8
            && ttHit
            && currMove == entry.bestMove
            && ctx->excluded == 0
            && entry.depth >= depth - 3
            && entry.nodeType != AllNode)
        {
            const int sBeta = std::max(-inf + 1, entry.score - depth * 2);
            const int sDepth = (depth - 1) / 2;

            ctx->excluded = currMove;
            const int singularScore = PVS<false, mode>(board, sDepth, sBeta-1, sBeta, ply, ctx, cutnode).score;
            ctx->excluded = Move();
            
            // Singular extension
            if (singularScore < sBeta) {
                extension++;

                if constexpr (!isPV) {
                    // Double extension
                    if (singularScore <= sBeta - 1 - 30) {
                        extension++;
                    }
                }
            }
            // Negative extension
            else if (entry.score >= beta) {
                extension--;
            }
        }

        // PVS SEE
        int SEEThreshold = currMove.IsQuiet() ? -80 * depth : -30 * depth * depth;

        if (ply && depth <= 10 && !SEE(board, currMove, SEEThreshold))
            continue;

        int reductions = GetReductions<isPV>(board, currMove, depth, moveSeen, ply, cutnode, ctx);

        int newDepth = depth + copy.InCheck() - 1 + extension;

        // First move (suspected PV node)
        if (!moveSeen) {
            // Full search
            if constexpr (isPV) {
                score = -PVS<isPV, mode>(copy, newDepth, -beta, -alpha, ply + 1, ctx, false).score;
            } else {
                score = -PVS<isPV, mode>(copy, newDepth, -beta, -alpha, ply + 1, ctx, !cutnode).score;
            }
        } else if (reductions) {
            // Null-window search with reductions
            score = -PVS<false, mode>(copy, newDepth - reductions, -alpha-1, -alpha, ply + 1, ctx, true).score;

            if (score > alpha) {
                // Null-window search now without the reduction
                score = -PVS<false, mode>(copy, newDepth, -alpha-1, -alpha, ply + 1, ctx, !cutnode).score;
            }
        } else {
            // Null-window search
            score = -PVS<false, mode>(copy, newDepth, -alpha-1, -alpha, ply + 1, ctx, !cutnode).score;
        }

        // Check if we need to do full window re-search
        if (moveSeen && score > alpha && score < beta) {
            score = -PVS<isPV, mode>(copy, newDepth, -beta, -alpha, ply + 1, ctx, false).score;
        }

        moveSeen++;

        if (ctx->searchStopped) return 0;

        if (currMove != 0 && currMove.IsQuiet()) {
            seenQuiets[seenQuietsCount] = currMove;
            seenQuietsCount++;
        }

        // Fail high (beta cutoff)
        if (score >= beta) {
            if (!currMove.IsCapture()) {
                ctx->killerMoves[1][ply] = ctx->killerMoves[0][ply];
                ctx->killerMoves[0][ply] = currMove;

                int bonus = 300 * depth - 250;

                ctx->history.Update(board.sideToMove, currMove, bonus);

                if (ply > 0) {
                    int prevType = ctx->ss[ply-1].pieceType;
                    int prevTo = ctx->ss[ply-1].moveTo;
                    int pieceType = ctx->ss[ply].pieceType;
                    int to = ctx->ss[ply].moveTo;
                    bool otherColor = ctx->ss[ply-1].side;

                    ctx->conthist.Update(board.sideToMove, otherColor, prevType, prevTo, pieceType, to, bonus);

                    // Malus
                    for (int moveIndex = 0; moveIndex < seenQuietsCount - 1; moveIndex++) {
                        int pieceType = board.GetPieceType(seenQuiets[moveIndex].MoveFrom());
                        int to = seenQuiets[moveIndex].MoveTo();

                        ctx->conthist.Update(board.sideToMove, otherColor, prevType, prevTo, pieceType, to, -bonus);
                    }

                    if (ply > 1) {
                        prevType = ctx->ss[ply-2].pieceType;
                        prevTo = ctx->ss[ply-2].moveTo;
                        otherColor = ctx->ss[ply-2].side;

                        ctx->conthist.Update(board.sideToMove, otherColor, prevType, prevTo, pieceType, to, bonus);

                        // Malus
                        for (int moveIndex = 0; moveIndex < seenQuietsCount - 1; moveIndex++) {
                            int pieceType = board.GetPieceType(seenQuiets[moveIndex].MoveFrom());
                            int to = seenQuiets[moveIndex].MoveTo();

                            ctx->conthist.Update(board.sideToMove, otherColor, prevType, prevTo, pieceType, to, -bonus);
                        }

                    }
                }


                // Malus
                for (int moveIndex = 0; moveIndex < seenQuietsCount - 1; moveIndex++) {
                    ctx->history.Update(board.sideToMove, seenQuiets[moveIndex], -bonus);
                }
            }

            if (!ctx->excluded)
                ctx->TT.WriteEntry(board.hashKey, depth, score, CutNode, currMove);
            return score;
        }

        results.score = std::max(score, results.score);

        if (score > alpha) {
            nodeType = PV;
            alpha = score;
            results.bestMove = currMove;
            ctx->pvLine.SetMove(ply, currMove);
        }
    }

    if (moveSeen == 0) {
        if (board.InCheck()) { // checkmate
            return -MATE_SCORE + ply;
        } else { // stalemate
            return 0;
        }
    }

    if (ctx->searchStopped) return 0;
    if (!ctx->excluded)

        if (!board.InCheck() && (results.bestMove.IsQuiet())
            && !(nodeType == CutNode && results.score <= staticEval)
            && !(nodeType == AllNode && results.score >= staticEval)) {

            int corrHistBonus = std::clamp(results.score - staticEval, -CORRHIST_LIMIT, CORRHIST_LIMIT);

            ctx->corrhist.UpdatePawnHist(board, depth, corrHistBonus);
        }

        ctx->TT.WriteEntry(board.hashKey, depth, results.score, nodeType, results.bestMove);
    return results;
}

class AspirationWindow {
public:
    int delta = 50;

    int alpha = -inf;
    int beta = inf;

    void WidenDown() {
        alpha -= delta;
        delta *= 2;
    }

    void WidenUp() {
        beta += delta;
        delta *= 2;
    }

    void Set(int score) {
        delta = 50;
        alpha = score - delta;
        beta = score + delta;
    }
};

// Iterative deepening
template <searchMode mode>
static SearchResults ID(Board &board, SearchParams params, SearchContext* ctx) {
    int fullTime = board.sideToMove ? params.btime : params.wtime;
    int inc = board.sideToMove ? params.binc : params.winc;
    int movesToGo = params.movesToGo ? params.movesToGo : 20;

    SearchResults safeResults;
    safeResults.score = -inf;

    int toDepth = MAX_DEPTH;

    if constexpr (mode == bench) {
        toDepth = BENCH_DEPTH;
    }

    AspirationWindow aw;

    int elapsed = 0;

    ctx->sw.Restart();

    for (int depth = 1; depth <= toDepth; depth++) {
        ctx->timeToSearch = std::max((fullTime / movesToGo) + (inc / 2), 4);
        int softTime = ctx->timeToSearch * 0.65;

        SearchResults currentResults = PVS<true, mode>(board, depth, aw.alpha, aw.beta, 0, ctx, false);
        elapsed = ctx->sw.GetElapsedMS();

        if (aw.alpha != -inf && currentResults.score <= aw.alpha) {
            aw.WidenDown();
            depth--;
            continue;
        }
        if (aw.beta != inf && currentResults.score >= aw.beta) {
            aw.WidenUp();
            depth--;
            continue;
        }

        aw.Set(currentResults.score);

        if (ctx->searchStopped) {
            break;
        } else {
            if (currentResults.bestMove) {
                safeResults = currentResults;
            }

            if constexpr (mode == normal || mode == nodesMode) {
                std::cout << "info ";
                std::cout << "depth " << depth;
                std::cout << " seldepth " << ctx->seldepth;
                std::cout << " time " << elapsed;
                std::cout << " score cp " << safeResults.score;
                std::cout << " nodes " << ctx->nodes << " nps " << int(ctx->nodes/ctx->sw.GetElapsedSec());
                std::cout << " hashfull " << ctx->TT.GetUsedPercentage();
                std::cout << " pv ";
                ctx->pvLine.Print(0);
                std::cout << std::endl;

                if constexpr (mode == normal) {
                    if (ctx->sw.GetElapsedMS() >= softTime) {
                        ctx->searchStopped = true;
                        break;
                    }
                }
            } else if constexpr (mode == datagen) {
                if (ctx->nodes >= DATAGEN::SOFT_NODES) {
                    ctx->searchStopped = true;
                    break;
                }
            }
        }
    }

    return safeResults;
}

template <searchMode mode>
SearchResults SearchPosition(Board &board, SearchParams params, SearchContext* ctx) {
    ctx->searchStopped = false;
    ctx->seldepth = 0;
    if constexpr (mode != bench) {
        ctx->nodes = 0;

        if constexpr (mode == nodesMode) {
            ctx->nodesToGo = params.nodes;
        }
    }
    ctx->pvLine.Clear();

    SearchResults results = ID<mode>(board, params, ctx);

    if constexpr (mode != normal) return results;

    std::cout << "bestmove ";
    results.bestMove.PrintMove();
    std::cout << std::endl;

    return results;
}

int MoveEstimatedValue(Board& board, Move& move) {
    int pieceType = board.GetPieceType(move.MoveTo());
    int value = pieceType != nullPieceType ? SEEPieceValues[pieceType] : 0;

    if (move.IsPromo()) {
        int promoPiece = -1;
        if (move.IsCapture()) {
            promoPiece = move.GetFlags() - 9;
        } else {
            promoPiece = move.GetFlags() - 5;
        }

        value += SEEPieceValues[promoPiece] - SEEPieceValues[Pawn];
    } else if (move.GetFlags() == epCapture) {
        value = SEEPieceValues[Pawn];
    } else if (move.GetFlags() == queenCastle || move.GetFlags() == kingCastle) {
        value = 0;
    }

    return value;
}

template SearchResults PVS<true, searchMode::bench>(Board&, int, int, int, int, SearchContext* ctx, bool cutnode);
template SearchResults PVS<false, searchMode::bench>(Board&, int, int, int, int, SearchContext* ctx, bool cutnode);
template SearchResults PVS<true, searchMode::normal>(Board&, int, int, int, int, SearchContext* ctx, bool cutnode);
template SearchResults PVS<false, searchMode::normal>(Board&, int, int, int, int, SearchContext* ctx, bool cutnode);
template SearchResults PVS<true, searchMode::datagen>(Board&, int, int, int, int, SearchContext* ctx, bool cutnode);
template SearchResults PVS<false, searchMode::datagen>(Board&, int, int, int, int, SearchContext* ctx, bool cutnode);

template SearchResults SearchPosition<normal>(Board &board, SearchParams params, SearchContext* ctx);
template SearchResults SearchPosition<bench>(Board &board, SearchParams params, SearchContext* ctx);
template SearchResults SearchPosition<datagen>(Board &board, SearchParams params, SearchContext* ctx);
template SearchResults SearchPosition<nodesMode>(Board &board, SearchParams params, SearchContext* ctx);

}
