#pragma once
#include <array>
#include "bitboards.h"

// pawn attacks table [side][square]
extern Bitboard pawnAttacks[2][64];
extern Bitboard knightAttacks[64];
extern Bitboard kingAttacks[64];

// generate pawn attacks
Bitboard maskPawnAttacks(int square, int side);

// generate knight attacks
Bitboard maskKnightAttacks(int square);

// generate king attacks
Bitboard maskKingAttacks(int square);

// generate bishop attacks
Bitboard maskBishopAttacks(int square);

// generate rook attacks
Bitboard maskRookAttacks(int square);

void initLeaperAttacks();