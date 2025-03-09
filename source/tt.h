#pragma once
#include "board.h"
#include "search.h"
#include <vector>

// 2[colors] * 6[pieces] * 64[squares]
extern U64 zKeys[2][6][64];

// There are 8 files
extern U64 zEnPassant[8];

// 16 possible castling right variations
extern U64 zCastle[16];

extern U64 zSide;

void InitZobrist();
U64 GetHashKey(Board &board);

class TTEntry {
public:
    U64 hashKey;
    int depth;
    int score;
    int nodeType;
    Move bestMove;
};

// 5 MB
inline int hashSize = 5000000 / sizeof(TTEntry);
inline std::vector<TTEntry> TTable(hashSize);

SearchResults ReadEntry(U64 &hashKey, int depth, int alpha, int beta);
void WriteEntry(U64 &hashKey, int depth, int score, int nodeType, Move besteMove);
