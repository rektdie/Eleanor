#pragma once
#include "board.h"

int NegaMax(Board board, int depth, int alpha, int beta);
Move NegaMaxHandler(Board &board, int depth);
void SearchPosition(Board &board, int depth);