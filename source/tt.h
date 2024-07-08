#pragma once
#include "board.h"

// 2[colors] * 6[pieces] * 64[squares]
extern U64 zKeys[2][6][64];

// There are 8 files
extern U64 zEnPassant[8];

// 16 possible castling right variations
extern U64 zCastle[16];

extern U64 zSide;

void InitZobrist();
U64 GetHashKey(Board &board);
