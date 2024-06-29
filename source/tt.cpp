#include "tt.h"
#include "types.h"
#include <iostream>

static inline U64 initialSeed = 0x60919C48E57863B9;

// 2[colors] * 6[pieces] * 64[squares]
static U64 zKeys[2][6][64];

// There are 8 files
static U64 zEnPassant[8];

static U64 zCastle[16];
static U64 zSide;

static U64 RandomNum() {
    initialSeed ^= initialSeed << 13;
    initialSeed ^= initialSeed >> 17;
    initialSeed ^= initialSeed << 5;
    return initialSeed;
}

void InitZobrist() {
    // init piece square keys 
    for (int i = White; i <= Black; i++) {
        for (int j = Pawn; j <= King; j++) {
            for (int k = a1; k <= h8; k++) {
                zKeys[i][j][k] = RandomNum();
            }
        } 
    }

    // init en passant file keys
    for (int i = 0; i < 8; i++) {
        zEnPassant[i] = RandomNum();
    }

    // init castling rights keys
    for (int i = 0; i < 16; i++) {
        zCastle[i] = RandomNum();
    }

    // init side key
    zSide = RandomNum();
}
