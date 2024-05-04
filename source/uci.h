#pragma once
#include "move.h"
#include "board.h"

// Returns Move object from a string
Move ParseMove(Board &board, const char* moveString);
void ParsePosition(Board &board, char* command);