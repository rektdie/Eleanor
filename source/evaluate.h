#pragma once
#include "board.h"

constexpr int materialValues[] {
    100,    // Pawn
    300,    // Knight
    350,    // Bishop
    500,    // Rook
    1000,   // Queen
    10000   // King
};

int Evaluate(Board &board);