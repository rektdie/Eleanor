#include "tt.h"

static U64 initialSeed = 0x60919C48E57863B9;

// 2[colors] * 6[pieces] * 64[squares]
U64 zKeys[2][6][64];

// There are 8 files
U64 zEnPassant[8];

// 16 possible castling right variations
U64 zCastle[16];

U64 zSide;

// PRNG using xorshift
static U64 RandomNum() {
    initialSeed ^= initialSeed << 13;
    initialSeed ^= initialSeed >> 7;
    initialSeed ^= initialSeed << 17;
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

U64 GetHashKey(Board &board) {
    U64 key = 0ULL;

    // XORing pieces
    for (int color = White; color <= Black; color++) {
        for (int piece = Pawn; piece <= King; piece++) {
            Bitboard pieceSet = board.pieces[piece] & board.colors[color];

            while (pieceSet) {
                int square = pieceSet.getLS1BIndex();

                key ^= zKeys[color][piece][square];

                pieceSet.PopBit(square);
            }
        }
    }

    // XORing en passant
    if (board.enPassantTarget != -1) {
        key ^= zEnPassant[board.enPassantTarget % 8];
    }

    // XORing castling rights
    key ^= zCastle[board.castlingRights];

    // XORing side to move
    if (board.sideToMove) key ^= zSide;

    return key;
}
