#pragma once

#include "search.h"
#include "movegen.h"

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

                    if (ttMove)
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
                        PickBest(index);

                        Move m = board.moveList[index++];

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
        TTEntry current = ctx->TT.GetEntry(board.hashKey);

        if (current.hashKey == board.hashKey && current.bestMove == move)
            return 100000;

        if (move.IsCapture()) {

            int attackerType = board.GetPieceType(move.MoveFrom());
            int targetType   = board.GetPieceType(move.MoveTo());

            if (move.GetFlags() == epCapture)
                targetType = Pawn;

            int capthistScore =
                ctx->capthist[board.sideToMove][attackerType][targetType][move.MoveTo()];

            return 50000 * SEE(board, move, seeOrderingThreshold)
                 + (100 * targetType - attackerType + 105)
                 + capthistScore;
        }

        if (ctx->killerMoves[0][ply] == move)
            return 41000;

        if (ctx->killerMoves[1][ply] == move)
            return 40000;

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

        return 20000 + historyScore + conthistScore;
    }

    void ScoreAllMoves() {
        int moveCount = board.currentMoveIndex;
        for (int i = 0; i < moveCount; i++)
            scores[i] = ScoreMove(board.moveList[i]);
    }

    void PickBest(int start) {
        int moveCount = board.currentMoveIndex;
        int bestIdx   = start;

        for (int i = start + 1; i < moveCount; i++) {
            if (scores[i] > scores[bestIdx])
                bestIdx = i;
        }

        if (bestIdx != start) {
            std::swap(board.moveList[start], board.moveList[bestIdx]);
            std::swap(scores[start],         scores[bestIdx]);
        }
    }
};