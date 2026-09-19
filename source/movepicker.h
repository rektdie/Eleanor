#pragma once
#include <array>
#include "board.h"
#include "movegen.h"

namespace SEARCH {

class SearchContext;

enum class PickStage {
    TTMove,
    GenNoisy,
    GoodNoisy,
    Killer,
    GenQuiet,
    Quiet,
    BadNoisy,
    Finished
};

template <MovegenMode mode>
class MovePicker {
public:
    MovePicker(Board &board, SearchContext *ctx, int ply, Move ttMove);

    Move Next();

private:
    Board &board;
    SearchContext *ctx;
    int ply;

    Move ttMove;
    Move killerMove;

    PickStage stage;

    int noisyStart = 0;
    int noisyEnd = 0;
    int goodNoisyEnd = 0;

    int quietStart = 0;
    int quietEnd = 0;

    int noisyCursor = 0;
    int badNoisyCursor = 0;
    int quietCursor = 0;

    std::array<int, MAX_MOVES> scores{};

    void ScoreNoisyRange();

    void ScoreQuietRange();

    int ScoreNoisyMove(Move move);
    int ScoreQuietMove(Move move);

    Move PopBest(int &cursor, int end);
};

}
