#pragma once
#include <array>
#include "bitboards.h"

// pawn attacks table [side][square]
extern Bitboard pawnAttacks[2][64];

// generate pawn attacks
Bitboard maskPawnAttacks(int square, int side);

void initLeaperAttacks();