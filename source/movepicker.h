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
        Rest,
        End
    };

    Board& board;
    SearchContext* ctx;
    Move ttMove;
    int ply;
    Stage stage;

    int index;
    bool generated;

    int scores[MAX_MOVES];

public:
    MovePicker(Board& b, SearchContext* c, int p, Move tt)
        : board(b), ctx(c), ttMove(tt), ply(p), stage(TT), index(0), generated(false)
    {
    }

    Move Next() {
        while (true) {
            switch (stage) {

                case TT: {
                    stage = Rest;

                    if (ttMove && board.IsPseudoLegal(ttMove))
                        return ttMove;
                    break;
                }

                case Rest: {

                    if (!generated) {
                        generated = true;

                        MOVEGEN::GenerateMoves<mode>(board, true);

                        ScoreAllMoves();
                        index = 0;
                    }

                    while (index < board.currentMoveIndex) {
                        int idx = FindNext();
                        Move m  = board.moveList[idx];

                        if (m == ttMove)
                            continue;

                        return m;
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

            return 500000 * SEE(board, move, seeOrderingThreshold)
                 + (100 * targetType - attackerType + 105)
                 + capthistScore;
        }

        if (ctx->killerMoves[ply] == move)
            return 410000;

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

            if (ply > 1) {
                conthistScore += ctx->conthist.GetNPly(board, move, ctx, ply, 2);

                if (ply > 3)
                    conthistScore += ctx->conthist.GetNPly(board, move, ctx, ply, 4);
            }
        }

        return 200000 + historyScore + conthistScore;
    }

    void ScoreAllMoves() {
        int moveCount = board.currentMoveIndex;
        for (int i = 0; i < moveCount; i++)
            scores[i] = ScoreMove(board.moveList[i]);
    }

    int FindNext() {
        const auto toU64 = [](int s) {
            int64_t widened = s;
            widened -= std::numeric_limits<int32_t>::min();
            return static_cast<uint64_t>(widened) << 32;
        };

        int moveCount = board.currentMoveIndex;

        uint64_t best = toU64(scores[index]) | static_cast<uint64_t>(256 - index);
        for (int i = index + 1; i < moveCount; i++) {
            uint64_t curr = toU64(scores[i]) | static_cast<uint64_t>(256 - i);
            if (curr > best)
                best = curr;
        }

        int bestIdx = 256 - static_cast<int>(best & 0xFFFFFFFF);

        if (bestIdx != index) {
            std::swap(board.moveList[index], board.moveList[bestIdx]);
            std::swap(scores[index],         scores[bestIdx]);
        }

        return index++;
    }
};