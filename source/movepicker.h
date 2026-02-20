#pragma once

#include "search.h"
#include "movegen.h"

#include <limits>

using namespace SEARCH;

template <MovegenMode mode>
class MovePicker {
private:
    enum Stage {
        TT,
        GEN_NOISY,
        GOOD_NOISY,
        GEN_QUIET,
        QUIETS,
        BAD_NOISY,
        End
    };

    Board&         board;
    SearchContext* ctx;
    Move           ttMove;
    int            ply;
    Stage          stage;

    int noisyEnd      = 0;
    int quietEnd      = 0;
    int index         = 0;
    int badNoisyCount = 0;
    int badNoisyIndex = 0;

    int badNoisyIndices[MAX_MOVES];
    int scores         [MAX_MOVES];

public:
    MovePicker(Board& b, SearchContext* c, int p, Move tt)
        : board(b), ctx(c), ttMove(tt), ply(p), stage(TT)
    {
    }

    Move Next() {
        while (true) {
            switch (stage) {

                case TT: {
                    stage = GEN_NOISY;
                    if (ttMove)
                        return ttMove;
                    break;
                }

                case GEN_NOISY: {
                    MOVEGEN::GenerateMoves<Noisy>(board, true);
                    noisyEnd = board.currentMoveIndex;
                    ScoreRange(0, noisyEnd);
                    index = 0;
                    stage = GOOD_NOISY;
                    break;
                }

                case GOOD_NOISY: {
                    while (index < noisyEnd) {
                        int  idx = FindNextInRange(index, noisyEnd);
                        Move m   = board.moveList[idx];
                        ++index;

                        if (m == ttMove)
                            continue;

                        if (!SEE(board, m, seeOrderingThreshold)) {
                            badNoisyIndices[badNoisyCount++] = idx;
                            continue;
                        }

                        return m;
                    }

                    stage = (mode == Noisy) ? BAD_NOISY : GEN_QUIET;
                    break;
                }

                case GEN_QUIET: {
                    board.currentMoveIndex = noisyEnd;
                    MOVEGEN::GenerateMoves<Quiet>(board, false);
                    quietEnd = board.currentMoveIndex;
                    ScoreRange(noisyEnd, quietEnd);
                    index = noisyEnd;
                    stage = QUIETS;
                    break;
                }

                case QUIETS: {
                    while (index < quietEnd) {
                        int  idx = FindNextInRange(index, quietEnd);
                        Move m   = board.moveList[idx];
                        ++index;

                        if (m == ttMove)
                            continue;

                        return m;
                    }

                    stage = BAD_NOISY;
                    break;
                }

                case BAD_NOISY: {
                    if (badNoisyIndex < badNoisyCount) {
                        int best = badNoisyIndex;
                        for (int i = badNoisyIndex + 1; i < badNoisyCount; i++) {
                            if (scores[badNoisyIndices[i]] > scores[badNoisyIndices[best]])
                                best = i;
                        }
                        if (best != badNoisyIndex)
                            std::swap(badNoisyIndices[badNoisyIndex], badNoisyIndices[best]);

                        return board.moveList[badNoisyIndices[badNoisyIndex++]];
                    }

                    stage = End;
                    break;
                }

                case End:
                    return Move();
            }
        }
    }

private:

    int ScoreMove(Move& move) {
        if (move.IsCapture()) {
            int attackerType = board.GetPieceType(move.MoveFrom());
            int targetType   = board.GetPieceType(move.MoveTo());

            if (move.GetFlags() == epCapture)
                targetType = Pawn;

            int capthistScore =
                ctx->capthist[board.sideToMove][attackerType][targetType][move.MoveTo()];

            return (100 * targetType - attackerType + 105) + capthistScore;
        }

        if (ctx->killerMoves[0][ply] == move) return 41000;
        if (ctx->killerMoves[1][ply] == move) return 40000;

        bool sourceThreatened = board.IsSquareThreatened(board.sideToMove, move.MoveFrom());
        bool targetThreatened = board.IsSquareThreatened(board.sideToMove, move.MoveTo());

        int historyScore =
            ctx->history[board.sideToMove]
                        [move.MoveFrom()]
                        [move.MoveTo()]
                        [sourceThreatened]
                        [targetThreatened];

        int conthistScore = 0;

        if (ply > 0) {
            conthistScore = ctx->conthist.GetNPly(board, move, ctx, ply, 1);
            if (ply > 1)
                conthistScore += ctx->conthist.GetNPly(board, move, ctx, ply, 2);
        }

        return historyScore + conthistScore;
    }

    void ScoreRange(int from, int to) {
        for (int i = from; i < to; i++)
            scores[i] = ScoreMove(board.moveList[i]);
    }

    int FindNextInRange(int from, int end) {
        const auto toU64 = [](int s) {
            int64_t widened = s;
            widened -= std::numeric_limits<int32_t>::min();
            return static_cast<uint64_t>(widened) << 32;
        };

        uint64_t best = toU64(scores[from]) | static_cast<uint64_t>(256 - from);
        for (int i = from + 1; i < end; i++) {
            uint64_t curr = toU64(scores[i]) | static_cast<uint64_t>(256 - i);
            if (curr > best)
                best = curr;
        }

        int bestIdx = 256 - static_cast<int>(best & 0xFFFFFFFF);

        if (bestIdx != from) {
            std::swap(board.moveList[from], board.moveList[bestIdx]);
            std::swap(scores[from],         scores[bestIdx]);
        }

        return from;
    }
};