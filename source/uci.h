#pragma once
#include "move.h"
#include "board.h"

class SearchParams {
public:
    int wtime = 0;
    int btime = 0;
    int winc = 0;
    int binc = 0;
    int movesToGo = 0;

    SearchParams(){}
};

// main UCI loop
void UCILoop(Board &board);
