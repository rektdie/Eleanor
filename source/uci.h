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

// Initializes the position with given parameters
void ParsePosition(Board &board, const char* command);

// Parse UCI "go" command
// GUI sends the engine this command if it should start searching for the best move
void ParseGo(Board &board, std::string &command);

// main UCI loop
void UCILoop(Board &board);
