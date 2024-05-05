#pragma once
#include "move.h"
#include "board.h"

// Returns Move object from a string
Move ParseMove(Board &board, const char* moveString);

// Initializes the position with given parameters
void ParsePosition(Board &board, const char* command);

// Parse UCI "go" command
// GUI sends the engine this command if it should start searching for the best move
void ParseGo(Board &board, const char* command);

// main UCI loop
void UCILoop(Board &board);