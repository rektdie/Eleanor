#pragma once
#include "bitboards.h"

// pawn attacks table [side][square]
extern Bitboard pawnAttacks[2][64];
extern Bitboard knightAttacks[64];
extern Bitboard kingAttacks[64];

// generate pawn masks
Bitboard maskPawnAttacks(int square, int side);

// generate knight masks
Bitboard maskKnightAttacks(int square);

// generate king masks
Bitboard maskKingAttacks(int square);

// generate bishop masks
Bitboard maskBishopAttacks(int square);

//generate bishop attacks
Bitboard bishopAttacksOnTheFly(int square, Bitboard block);

// generate rook masks
Bitboard maskRookAttacks(int square);

//generate rook attacks
Bitboard rookAttacksOnTheFly(int square, Bitboard block);

void initLeaperAttacks();