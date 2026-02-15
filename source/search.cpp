#include "search.h"
#include "movegen.h"
#include <algorithm>
#include <cmath>
#include "datagen.h"
#include "benchmark.h"
#include "types.h"
#include "tunables.h"
#include "termcolor.hpp"
#include <iomanip>

namespace SEARCH {

void InitLMRTable() {
    for (int i = 0; i < 2; i++) {
        const double base = i ? lmrBaseQuiet : lmrBaseNoisy;
        const double divisor = i ? lmrDivisorQuiet : lmrDivisorNoisy;

        for (int depth = 0; depth <= MAX_DEPTH; depth++) {
            for (int move = 0; move < MAX_MOVES; move++) {
                if (depth == 0 || move == 0) {
                    lmrTable[i][depth][move] = 0;
                } else {
                    lmrTable[i][depth][move] = std::floor(base + std::log(depth) * std::log(move) / divisor);
                }
            }
        }
    }
}

int16_t ContHistory::GetNPly(Board& board, Move& move, SearchContext* ctx, int ply, int n) {
    int prevType = ctx->ss[ply-n].pieceType;
    int prevTo = ctx->ss[ply-n].moveTo;
    int pieceType = board.GetPieceType(move.MoveFrom());
    int to = move.MoveTo();
    bool otherColor = ctx->ss[ply-n].side;

    return ctx->conthist[otherColor][prevType][prevTo][board.sideToMove][pieceType][to];
}

static int AdjustEval(Board &board, SearchContext* ctx, int eval) {
    int corrhist = ctx->corrhist.GetAllHist(board);

    int mateFound = MATE_SCORE - MAX_DEPTH;

    return std::clamp(eval + corrhist / CORRHIST_GRAIN, -mateFound + 1, mateFound - 1);
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

        const int capthistScore = ctx->capthist[board.sideToMove][attackerType][targetType][move.MoveTo()];

        return 50000 * ((SEE(board, move, seeOrderingThreshold))) + (100 * targetType - attackerType + 105) + capthistScore;
    } else {
        if (ctx->killerMoves[0][ply] == move) {
            return 41000;
        } else if (ctx->killerMoves[1][ply] == move) {
            return 40000;
        } else {
            bool sourceThreatened = board.IsSquareThreatened(board.sideToMove, move.MoveFrom());
            bool targetThreatened = board.IsSquareThreatened(board.sideToMove, move.MoveTo());

            int historyScore = ctx->history[board.sideToMove][move.MoveFrom()][move.MoveTo()][sourceThreatened][targetThreatened];
            int conthistScore = 0;

            if (ply > 0) {
                conthistScore = ctx->conthist.GetNPly(board, move, ctx, ply, 1);
                
                if (ply > 1) {
                    conthistScore += ctx->conthist.GetNPly(board, move, ctx, ply, 2);
                }
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
static int GetReductions(Board &board, Move &move, int depth, int moveSeen, int ply, bool cutnode, bool improving, bool corrplexity, bool ttpv, SearchContext* ctx) {
    int reduction = 0;

    // Late Move Reduction
    if (depth >= 2 && moveSeen >= 2 + (2 * isPV)) {
        reduction = lmrTable[move.IsQuiet()][depth][moveSeen] * 1024;

        if (cutnode)
            reduction += lmrCutnode;

        if constexpr (isPV)
            reduction -= lmrIsPV;
    
        if (!improving)
            reduction += lmrImproving;

        if (corrplexity)
            reduction -= lmrCorrplexity;

        if (ttpv)
            reduction -= lmrTTPV;

        // History LMR
        int historyReduction = 0;

        if (move.IsQuiet()) {
            bool sourceThreatened = board.IsSquareThreatened(board.sideToMove, move.MoveFrom());
            bool targetThreatened = board.IsSquareThreatened(board.sideToMove, move.MoveTo());

            historyReduction = ctx->history[board.sideToMove][move.MoveFrom()][move.MoveTo()][sourceThreatened][targetThreatened];
            if (ply > 0) {
                historyReduction += ctx->conthist.GetNPly(board, move, ctx, ply, 1);

                if (ply > 1)
                    historyReduction += ctx->conthist.GetNPly(board, move, ctx, ply, 2);
            }
        } else if (move.IsCapture()) {
            int attackerType = board.GetPieceType(move.MoveFrom());
            int targetType = board.GetPieceType(move.MoveTo());

            if (move.GetFlags() == epCapture) {
                targetType = Pawn;
            }
            
            historyReduction = ctx->capthist[board.sideToMove][attackerType][targetType][move.MoveTo()];
        }

        historyReduction = historyReduction / lmrHistoryDivisor;
        reduction -= historyReduction * 1024;
    }

    reduction /= 1024;

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
static bool ShouldStop(SearchContext* ctx) {
    if constexpr (mode == normal) {
        if (ctx->nodes % 1024 == 0) {
            if (ctx->sw.GetElapsedMS() >= ctx->timeToSearch) {
                ctx->searchStopped = true;
                return true;
            }
        }
    } else if constexpr (mode == nodesMode) {
        if (ctx->nodes > ctx->nodesToGo) {
            ctx->searchStopped = true;
            return true;
        }
    } else if constexpr (mode == datagen) {
        if (ctx->nodes > DATAGEN::HARD_NODES) {
            ctx->searchStopped = true;
            return true;
        }
    }

    return false;
}

template <bool isPV, searchMode mode>
static SearchResults Quiescence(Board& board, int alpha, int beta, int ply, SearchContext* ctx) {
    if (ShouldStop<mode>(ctx)) return 0;

    if (ply >= MAX_DEPTH)
        return NNUE::net.Evaluate(board);

    if (ply > ctx->seldepth)
        ctx->seldepth = ply;

    int staticEval = AdjustEval(board, ctx, NNUE::net.Evaluate(board));
    ctx->ss[ply].eval = staticEval;

    int bestScore = staticEval;

    TTEntry entry;
    if (!ctx->excluded) {
        entry = ctx->TT.GetRawEntry(board.hashKey);
        if (entry.hashKey == board.hashKey) {
            bestScore = entry.score;
        }
    }

    const bool ttpv = isPV | entry.ttpv;


    if (bestScore >= beta) {
        if (!ctx->excluded) {
            ctx->TT.WriteEntry(board.hashKey, 0, staticEval, CutNode, Move(), ttpv);
        }
        return bestScore;
    }

    if (alpha < bestScore) {
        alpha = bestScore;
    }

    const int fpScore = bestScore + 100;

    MOVEGEN::GenerateMoves<Noisy>(board, true);

    SortMoves(board, ply, ctx);

    SearchResults results;

    int nodeType = AllNode;

    for (int i = 0; i < board.currentMoveIndex; i++) {
        // QS FP
        if (!board.InCheck() && board.moveList[i].IsCapture() &&
            fpScore <= alpha && !SEE(board, board.moveList[i], 1)) {

            bestScore = std::max(bestScore, fpScore);
            continue;
        }

        Board copy = board;
        int isLegal = copy.MakeMove(board.moveList[i]);
        if (!isLegal) continue;

        ctx->ss[ply].pieceType = board.GetPieceType(board.moveList[i].MoveFrom());
        ctx->ss[ply].moveTo = board.moveList[i].MoveTo();
        ctx->ss[ply].side = board.sideToMove;

        if (!SEE(board, board.moveList[i], seeQsThreshold))
            continue;

        if (copy.positionIndex >= ctx->positionHistory.size()) {
            ctx->positionHistory.resize(copy.positionIndex + 100);
        }
        ctx->positionHistory[copy.positionIndex] = copy.hashKey;
        ctx->nodes++;

        ctx->TT.PrefetchEntry(copy.hashKey);

        int score = -Quiescence<isPV, mode>(copy, -beta, -alpha, ply + 1, ctx).score;

        if (score >= beta) {
            if (!ctx->excluded) {
                ctx->TT.WriteEntry(board.hashKey, 0, score, CutNode, board.moveList[i], ttpv);
            }
            return score;
        }

        bestScore = std::max(score, bestScore);

        if (score > alpha) {
            nodeType = PV;
            alpha = score;
            results.bestMove = board.moveList[i];
        }
    }

    results.score = bestScore;
    if (ctx->searchStopped) return 0;
    if (!ctx->excluded) {
        ctx->TT.WriteEntry(board.hashKey, 0, results.score, nodeType, results.bestMove, ttpv);
    }
    return results;
}

template <bool isPV, searchMode mode>
SearchResults PVS(Board& board, int depth, int alpha, int beta, int ply, SearchContext* ctx, bool cutnode) {
    if (ShouldStop<mode>(ctx)) return 0;

    if (ply >= MAX_DEPTH)
        return NNUE::net.Evaluate(board);

    if (ply > ctx->seldepth)
        ctx->seldepth = ply;


    ctx->pvLine.SetLength(ply);
    if (ply && (IsDraw(board, ctx))) return 0;


    TTEntry entry;
    if (!ctx->excluded)
        entry = ctx->TT.GetRawEntry(board.hashKey);

    const bool ttHit = entry.hashKey == board.hashKey;
    const bool ttpv = isPV | entry.ttpv;

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

    if (depth <= 0) return Quiescence<isPV, mode>(board, alpha, beta, ply, ctx);

    int rawEval = NNUE::net.Evaluate(board);
    const int staticEval = AdjustEval(board, ctx, rawEval);
    ctx->ss[ply].eval = staticEval;

    int ttAdjustedEval = staticEval;

    if (!ctx->excluded && !board.InCheck() && ttHit && 
        ((entry.nodeType == PV) ||
        (entry.nodeType == AllNode && entry.score <= staticEval) ||
        (entry.nodeType == CutNode && entry.score >= staticEval))) {

        ttAdjustedEval = entry.score;
    }

    bool corrplexity = std::abs(rawEval - staticEval) > 70;

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
            int margin = rfpBase + rfpMargin * (depth - improving);
            if (ttAdjustedEval - margin >= beta && depth < 7) {
                return (beta + (ttAdjustedEval - beta) / 3);
            }

            // Razoring
            if (!isPV && depth <= 3 && staticEval + razoringScalar * depth < alpha) {
                return Quiescence<isPV, mode>(board, alpha, beta, ply, ctx).score;
            }

            // Null Move Pruning
            if (!ctx->doingNullMove && staticEval >= beta + nmpBetaMargin) {
                if (depth > 1 && !board.InPossibleZug()) {
                    Board copy = board;
                    copy.MakeMove(Move());

                    const int reduction = 4 + improving + depth / 3;

                    ctx->TT.PrefetchEntry(copy.hashKey);

                    ctx->doingNullMove = true;
                    int score = -PVS<false, mode>(copy, depth - reduction, -beta, -beta + 1, ply + 1, ctx, !cutnode).score;
                    ctx->doingNullMove = false;

                    if (ctx->searchStopped) return 0;
                    if (score >= beta) return score;
                }
            }
        }
    }

    // Probcut
    const int probcutBeta = beta + 200;
    const int probcutDepth = std::max(depth - 3, 1);

    if (depth >= 7 && std::abs(beta) < MATE_SCORE - MAX_DEPTH
        && (!entry.bestMove || !entry.bestMove.IsQuiet())
        && !(ttHit && entry.depth >= probcutDepth && entry.score < probcutBeta)) {

        MOVEGEN::GenerateMoves<Noisy>(board, true);
        SortMoves(board, ply, ctx);

        const int seeThreshold = (probcutBeta - staticEval) * 15 / 16;

        for (int i = 0; i < board.currentMoveIndex; i++) {
            Move currMove = board.moveList[i];

            if (ctx->excluded == currMove)
                continue;

            if (!SEE(board, currMove, seeThreshold))
                continue;

            Board copy = board;
            bool isLegal = copy.MakeMove(currMove);

            if (!isLegal) continue;

            if (copy.positionIndex >= ctx->positionHistory.size()) {
                ctx->positionHistory.resize(copy.positionIndex + 100);
            }
            ctx->positionHistory[copy.positionIndex] = copy.hashKey;
            ctx->nodes++;

            int score = -Quiescence<isPV, mode>(copy, -probcutBeta, -probcutBeta + 1, ply + 1, ctx).score;

            if (score >= probcutBeta) {
                score = -PVS<isPV, mode>(copy, probcutDepth - 1, -probcutBeta, -probcutBeta + 1,
                    ply + 1, ctx, !cutnode).score;
            }

            if (ctx->searchStopped) return 0;

            if (score >= probcutBeta) {
                ctx->TT.WriteEntry(board.hashKey, probcutDepth, score, CutNode, currMove, ttpv);

                return score;
            }
        }
        MOVEGEN::GenerateMoves<Quiet>(board, false);
    } else {
        MOVEGEN::GenerateMoves<All>(board, true);
    }

    SortMoves(board, ply, ctx);

    int score = -inf;
    int nodeType = AllNode;
    SearchResults results(-inf);

    int moveSeen = 0;
    std::array<Move, MAX_MOVES> seenQuiets;
    std::array<Move, MAX_MOVES> seenCaptures;
    int seenQuietsCount = 0;
    int seenCapturesCount = 0;

    // For all moves
    for (int i = 0; i < board.currentMoveIndex; i++) {
        Move currMove = board.moveList[i];

        if (ctx->excluded == currMove)
            continue;

        bool notMated = results.score > (-MATE_SCORE + MAX_DEPTH);
        int lmrDepth = depth - lmrTable[currMove.IsQuiet()][depth][moveSeen];

        // Late move pruning
        // If we are near a leaf node we prune moves
        // that are late in the list
        if (currMove.IsQuiet() && notMated) {

            int lmpThreshold = 7 + depth * depth * (1 + improving);

            if (moveSeen >= lmpThreshold) {
                continue;
            }
        }

        bool sourceThreatened = board.IsSquareThreatened(board.sideToMove, currMove.MoveFrom());
        bool targetThreatened = board.IsSquareThreatened(board.sideToMove, currMove.MoveTo());

        // Futility pruning
        // If our static eval is far below alpha, there is only a small chance
        // that a quiet move will help us so we skip them
        int historyScore = ctx->history[board.sideToMove][currMove.MoveFrom()][currMove.MoveTo()][sourceThreatened][targetThreatened];
        
        if (ply > 0) {
            historyScore += ctx->conthist.GetNPly(board, currMove, ctx, ply, 1);
            
            if (ply > 1)
                historyScore += ctx->conthist.GetNPly(board, currMove, ctx, ply, 2);
        }
        
        int margin = fpMargin * (lmrDepth + improving) + historyScore / 32;

        if (!isPV && ply && currMove.IsQuiet()
                && lmrDepth <= 5 && staticEval + margin < alpha && notMated) {
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

        ctx->TT.PrefetchEntry(copy.hashKey);

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
                    if (singularScore <= sBeta - 1 - doubleExtMargin) {
                        extension++;
                    }
                }
            }
            // Multicut
            else if (sBeta >= beta) {
                return sBeta;
            }
            // Negative extension
            else if (entry.score >= beta) {
                extension--;
            } else if (cutnode) {
                extension--;
            }
        }

        cutnode |= extension < 0;

        // PVS SEE
        int SEEThreshold = currMove.IsQuiet() ? seeQuietThreshold * depth : seeNoisyThreshold * depth * depth;

        if (ply && depth <= 10 && !SEE(board, currMove, SEEThreshold))
            continue;

        int reductions = GetReductions<isPV>(board, currMove, depth, moveSeen, ply, cutnode, improving, corrplexity, ttpv, ctx);

        int newDepth = depth + (copy.InCheck() && !ctx->excluded) - 1 + extension;

        U64 nodesBeforeSearch = ctx->nodes;

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

        // Root
        if (!ply) {
            ctx->nodesTable[currMove % 4096] += ctx->nodes - nodesBeforeSearch;
        }

        if (ctx->searchStopped) return 0;

        if (currMove != 0 && currMove.IsQuiet()) {
            seenQuiets[seenQuietsCount] = currMove;
            seenQuietsCount++;
        } else if (currMove != 0 && currMove.IsCapture()) {
            seenCaptures[seenCapturesCount] = currMove;
            seenCapturesCount++;
        }

        // Fail high (beta cutoff)
        if (score >= beta) {
            int bonus = historyBonusMultiplier * depth - historyBonusSub;

            if (!currMove.IsCapture()) {
                ctx->killerMoves[1][ply] = ctx->killerMoves[0][ply];
                ctx->killerMoves[0][ply] = currMove;

                ctx->history.Update(board.sideToMove, currMove, sourceThreatened, targetThreatened, bonus);

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
                    sourceThreatened = board.IsSquareThreatened(board.sideToMove, seenQuiets[moveIndex].MoveFrom());
                    targetThreatened = board.IsSquareThreatened(board.sideToMove, seenQuiets[moveIndex].MoveTo());

                    ctx->history.Update(board.sideToMove, seenQuiets[moveIndex], sourceThreatened, targetThreatened, -bonus);
                }
            } else {
                int movingPiece = board.GetPieceType(currMove.MoveFrom());
                int capturedPiece = board.GetPieceType(currMove.MoveTo());

                if (currMove.GetFlags() == epCapture) {
                    capturedPiece = Pawn;
                }

                ctx->capthist.Update(board.sideToMove, movingPiece, capturedPiece, currMove.MoveTo(), bonus);
            }

            // Capthist malus
            for (int moveIndex = 0; moveIndex < seenCapturesCount - currMove.IsCapture(); moveIndex++) {
                int movingPiece = board.GetPieceType(seenCaptures[moveIndex].MoveFrom());
                int capturedPiece = board.GetPieceType(seenCaptures[moveIndex].MoveTo());

                if (seenCaptures[moveIndex].GetFlags() == epCapture) {
                    capturedPiece = Pawn;
                }

                if (movingPiece == nullPieceType || capturedPiece == nullPieceType) {
                    std::cout << "asd\n";
                }

                ctx->capthist.Update(board.sideToMove, movingPiece, capturedPiece,
                    seenCaptures[moveIndex].MoveTo(), -bonus);
            }

            if (!ctx->excluded)
                ctx->TT.WriteEntry(board.hashKey, depth, score, CutNode, currMove, ttpv);
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
    if (!ctx->excluded) {

        if (!board.InCheck() && ((results.bestMove.IsQuiet() || !results.bestMove))
                && !(nodeType == AllNode && results.score >= staticEval)) {

            int corrHistBonus = std::clamp(results.score - staticEval, -CORRHIST_LIMIT, CORRHIST_LIMIT);

            ctx->corrhist.UpdateAll(board, depth, corrHistBonus);
        }

        ctx->TT.WriteEntry(board.hashKey, depth, results.score, nodeType, results.bestMove, ttpv);
    }
    return results;
}

class AspirationWindow {
public:
    int delta = aspInitialDelta;

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
        delta = aspInitialDelta;
        alpha = score - delta;
        beta = score + delta;
    }
};

static double ScaleTime(SearchContext *ctx, Move &move) {
    double notBmNodesFraction = 
       ctx->nodesTable[move % 4096] / double(ctx->nodes);
    double nodeScalingFactor = (1.5f - notBmNodesFraction) * 1.35f;
    
    return nodeScalingFactor;
}

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

    double nodeScaling = 1;

    if (!UCIEnabled && (mode == normal || mode == nodesMode)) {
        std::cout << termcolor::bold;
        std::cout << std::setw(6) << std::left << "Depth" 
                  << std::setw(10) << std::right << "Time"
                  << std::setw(9) << std::right << "Score"
                  << std::setw(11) << std::right << "Nodes"
                  << std::setw(12) << std::right << "NPS"
                  << "  PV" << std::endl;
        std::cout << termcolor::reset;
        std::cout << std::string(70, '-') << std::endl;
    }

    ctx->sw.Restart();

    for (int depth = 1; depth <= toDepth; depth++) {
        ctx->timeToSearch = std::max((fullTime / movesToGo) + (inc / 2), 4);
        int softTime = ctx->timeToSearch * 0.65 * nodeScaling;
        ctx->seldepth = 0;

        SearchResults currentResults = PVS<true, mode>(board, depth, aw.alpha, aw.beta, 0, ctx, false);

        elapsed = ctx->sw.GetElapsedMS();

        if (depth > 6) {
            nodeScaling = ScaleTime(ctx, currentResults.bestMove);
        }

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
                PrintSearchInfo(ctx, safeResults, depth, elapsed);

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
    ctx->nodesTable = {};
    if constexpr (mode != bench) {
        ctx->nodes = 0;

        if constexpr (mode == nodesMode) {
            ctx->nodesToGo = params.nodes;
        }
    }
    ctx->pvLine.Clear();

    SearchResults results = ID<mode>(board, params, ctx);

    if constexpr (mode != normal && mode != nodesMode) return results;

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

void PrintSearchInfo(SearchContext* ctx, SearchResults& results, int depth, int elapsed) {
    if (UCIEnabled) {
        std::cout << "info ";
        std::cout << "depth " << depth;
        std::cout << " seldepth " << ctx->seldepth;
        std::cout << " time " << elapsed;
        std::cout << " score ";

        if (std::abs(results.score) + MAX_DEPTH >= MATE_SCORE) {
            int mateIn = (MATE_SCORE - (std::abs(results.score) - 1)) / 2;
            mateIn  = results.score < 0 ? mateIn * -1 : mateIn;

            std::cout << "mate " << mateIn;
        } else {
            std::cout << "cp " << results.score;
        }

        std::cout << " nodes " << ctx->nodes << " nps " << int(ctx->nodes/ctx->sw.GetElapsedSec());
        std::cout << " hashfull " << ctx->TT.GetUsedPercentage();
        std::cout << " pv ";
        ctx->pvLine.Print(0, depth % 2 == 0);
        std::cout << std::endl;
    } else {
        if (depth % 2 == 0) {
            std::cout << termcolor::color<247>;
        } else {
            std::cout << termcolor::color<251>;
        }
        
        std::stringstream depthStr;
        depthStr << depth << '/' << ctx->seldepth;
        std::cout << std::setw(6) << std::left << depthStr.str();
        
        std::stringstream timeStr;
        if (elapsed >= 1000) {
            timeStr << std::fixed << std::setprecision(1) << (elapsed / 1000.0) << "s";
        } else {
            timeStr << elapsed << "ms";
        }
        std::cout << std::setw(10) << std::right << timeStr.str();
        
        std::stringstream scoreStr;
        bool isMate = false;
        int mateIn = 0;

        if (std::abs(results.score) + MAX_DEPTH >= MATE_SCORE) {
            isMate = true;
            mateIn = (MATE_SCORE - (std::abs(results.score) - 1)) / 2;
            mateIn = results.score < 0 ? mateIn * -1 : mateIn;
            if (mateIn < 0) {
                scoreStr << "-M" << std::abs(mateIn);
            } else {
                scoreStr << "+M" << mateIn;
            }
        } else {
            scoreStr << std::showpos << std::fixed << std::setprecision(2) 
                     << (results.score / 100.0) << std::noshowpos;
        }

        if (isMate) {
            if (mateIn > 0) {
                std::cout << termcolor::bright_green;
            } else {
                std::cout << termcolor::bright_red;
            }
        } else {
            if (results.score > 0) {
                std::cout << termcolor::green;
            } else if (results.score < 0) {
                std::cout << termcolor::red;
            }
        }
        
        std::cout << std::setw(9) << std::right << scoreStr.str();
        std::cout << termcolor::reset;
        if (depth % 2 == 0) {
            std::cout << termcolor::color<247>;
        } else {
            std::cout << termcolor::color<251>;
        }
        
        std::stringstream nodesStr;
        if (ctx->nodes >= 1000000) {
            nodesStr << std::fixed << std::setprecision(1) << (ctx->nodes / 1000000.0) << "M";
        } else if (ctx->nodes >= 1000) {
            nodesStr << std::fixed << std::setprecision(1) << (ctx->nodes / 1000.0) << "k";
        } else {
            nodesStr << ctx->nodes;
        }
        std::cout << std::setw(11) << std::right << nodesStr.str();
        
        int nps = int(ctx->nodes/ctx->sw.GetElapsedSec());
        std::stringstream npsStr;
        if (nps >= 1000000) {
            npsStr << std::fixed << std::setprecision(1) << (nps / 1000000.0) << "M/s";
        } else if (nps >= 1000) {
            npsStr << std::fixed << std::setprecision(1) << (nps / 1000.0) << "k/s";
        } else {
            npsStr << nps << "/s";
        }
        std::cout << std::setw(12) << std::right << npsStr.str();
        
        std::cout << "  ";
        ctx->pvLine.Print(0, depth % 2 == 0);
        
        std::cout << termcolor::reset << std::endl;
    }
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
