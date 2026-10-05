#pragma once
#include <atomic>
#include <string_view>
#include <vector>
#include "move.h"
#include "board.h"

inline bool UCIEnabled = false;
inline bool UCIShowWDL = false;
inline int threads = 1;
inline std::atomic<bool> searchStopped = false;

constexpr std::string_view EleanorVersion = "4.1";

class SearchParams {
public:
    int wtime = 0;
    int btime = 0;
    int winc = 0;
    int binc = 0;
    int movesToGo = 0;
    int nodes = 0;
    int depth = 0;
    int mate = 0;
    int movetime = 0;
    bool infinite = false;
    std::vector<Move> searchMoves;

    int threads = 1;

    SearchParams(){}
};

// main UCI loop
void UCILoop(Board &board);
