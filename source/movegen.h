#pragma once
#include "bitboards.h"

// bishop relevant occupancy bit count
constexpr int bishopRelevantBits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6
};

// rook relevant occupancy bit count
constexpr int rookRelevantBits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12
};

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