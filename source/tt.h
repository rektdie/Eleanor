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
inline int hashSize = (5 * 1000000) / sizeof(TTEntry);
constexpr int invalidEntry = 111111;

class TTable {
private:
    std::vector<TTEntry> table;
public:
    TTable() {
        table.reserve(hashSize);
    }

    void Resize(int size) {
        table.resize(size);
    }

    void Clear() {
        table.clear();
    }

    TTEntry* GetRawEntry(U64 &hashKey) {
        TTEntry *current = &table[hashKey % hashSize];
        return current;
    }

    SearchResults ReadEntry(U64 &hashKey, int depth, int alpha, int beta);

    void WriteEntry(U64 &hashKey, int depth, int score, int nodeType, Move besteMove);
};

inline TTable TT;