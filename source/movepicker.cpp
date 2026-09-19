#include "movepicker.h"
#include "search.h"
#include "tunables.h"
#include <utility>

namespace SEARCH {

template <MovegenMode mode>
MovePicker<mode>::MovePicker(Board &b, SearchContext *c, int p, Move tt)
    : board(b), ctx(c), ply(p), ttMove(tt), killerMove(), stage(PickStage::TTMove) {

    if (!ttMove || !board.IsPseudoLegal(ttMove)) {
        ttMove = Move();
    }

    if constexpr (mode == Noisy) {
        if (ttMove && !(ttMove.IsCapture() || ttMove.IsPromo())) {
            ttMove = Move();
        }
    } else {
        Move killer = ctx->killerMoves[ply];

        if (killer && killer != ttMove && !killer.IsCapture() && !killer.IsPromo()
                && board.IsPseudoLegal(killer)) {
            killerMove = killer;
        }
    }
}

template <MovegenMode mode>
int MovePicker<mode>::ScoreNoisyMove(Move move) {
    int score = MoveEstimatedValue(board, move) * noisyScoreScale;

    if (move.IsCapture()) {
        int attackerType = board.GetPieceType(move.MoveFrom());
        int targetType = (move.GetFlags() == epCapture) ? Pawn : board.GetPieceType(move.MoveTo());

        score += ctx->capthist[board.sideToMove][attackerType][targetType][move.MoveTo()];
    }

    return score;
}

template <MovegenMode mode>
int MovePicker<mode>::ScoreQuietMove(Move move) {
    bool sourceThreatened = board.IsSquareThreatened(board.sideToMove, move.MoveFrom());
    bool targetThreatened = board.IsSquareThreatened(board.sideToMove, move.MoveTo());

    int score = ctx->history[board.sideToMove][move.MoveFrom()][move.MoveTo()][sourceThreatened][targetThreatened];

    if (ply > 0) score += ctx->conthist.GetNPly(board, move, ctx, ply, 1);
    if (ply > 1) score += ctx->conthist.GetNPly(board, move, ctx, ply, 2);

    return score;
}

template <MovegenMode mode>
void MovePicker<mode>::ScoreNoisyRange() {
    noisyStart = 0;

    MOVEGEN::GenerateMoves<Noisy>(board, true, false);

    int rawEnd = board.currentMoveIndex;
    int write = noisyStart;

    for (int read = noisyStart; read < rawEnd; read++) {
        Move m = board.moveList[read];
        if (m == ttMove || m == killerMove) continue;

        board.moveList[write] = m;
        scores[write] = ScoreNoisyMove(m);
        write++;
    }

    noisyEnd = write;
    board.currentMoveIndex = noisyEnd;

    int goodWrite = noisyStart;

    for (int i = noisyStart; i < noisyEnd; i++) {
        Move m = board.moveList[i];

        if (SEE(board, m, seeOrderingThreshold)) {
            if (i != goodWrite) {
                std::swap(board.moveList[i], board.moveList[goodWrite]);
                std::swap(scores[i], scores[goodWrite]);
            }
            goodWrite++;
        }
    }

    goodNoisyEnd = goodWrite;
    noisyCursor = noisyStart;
    badNoisyCursor = goodNoisyEnd;
}

template <MovegenMode mode>
void MovePicker<mode>::ScoreQuietRange() {
    quietStart = noisyEnd;

    MOVEGEN::GenerateMoves<Quiet>(board, false, false);

    int rawEnd = board.currentMoveIndex;
    int write = quietStart;

    for (int read = quietStart; read < rawEnd; read++) {
        Move m = board.moveList[read];
        if (m == ttMove || m == killerMove) continue;

        board.moveList[write] = m;
        scores[write] = ScoreQuietMove(m);
        write++;
    }

    quietEnd = write;
    board.currentMoveIndex = quietEnd;
    quietCursor = quietStart;
}

template <MovegenMode mode>
Move MovePicker<mode>::PopBest(int &cursor, int end) {
    if (cursor >= end) return Move();

    int bestIdx = cursor;

    for (int i = cursor + 1; i < end; i++) {
        if (scores[i] > scores[bestIdx]) bestIdx = i;
    }

    if (bestIdx != cursor) {
        std::swap(board.moveList[bestIdx], board.moveList[cursor]);
        std::swap(scores[bestIdx], scores[cursor]);
    }

    Move best = board.moveList[cursor];
    cursor++;

    return best;
}

template <MovegenMode mode>
Move MovePicker<mode>::Next() {
    while (true) {
        switch (stage) {
        case PickStage::TTMove: {
            stage = PickStage::GenNoisy;
            if (ttMove) return ttMove;
            continue;
        }

        case PickStage::GenNoisy: {
            ScoreNoisyRange();
            stage = PickStage::GoodNoisy;
            continue;
        }

        case PickStage::GoodNoisy: {
            Move m = PopBest(noisyCursor, goodNoisyEnd);
            if (m) return m;

            if constexpr (mode == Noisy) {
                stage = PickStage::BadNoisy;
            } else {
                stage = PickStage::Killer;
            }
            continue;
        }

        case PickStage::Killer: {
            stage = PickStage::GenQuiet;
            if (killerMove) return killerMove;
            continue;
        }

        case PickStage::GenQuiet: {
            ScoreQuietRange();
            stage = PickStage::Quiet;
            continue;
        }

        case PickStage::Quiet: {
            Move m = PopBest(quietCursor, quietEnd);
            if (m) return m;

            stage = PickStage::BadNoisy;
            continue;
        }

        case PickStage::BadNoisy: {
            Move m = PopBest(badNoisyCursor, noisyEnd);
            if (m) return m;

            stage = PickStage::Finished;
            return Move();
        }

        case PickStage::Finished:
            return Move();

        default:
            return Move();
        }
    }
}

template class MovePicker<Noisy>;
template class MovePicker<All>;

}
