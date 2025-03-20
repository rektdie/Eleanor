#pragma once
#include "board.h"
#include "search.h"
#include <vector>

class TTEntry {
public:
    U64 hashKey;
    int depth;
    int score;
    int nodeType;
    Move bestMove;

    operator U64() {
        return hashKey;
    }
};

// 64 MB
constexpr U64 maxHash = (64* 1000000) / sizeof(TTEntry);
// 5 MB
constexpr U64 defaultHash = (5 * 1000000) / sizeof(TTEntry);

inline U64 hashSize = defaultHash;

constexpr int invalidEntry = 111111;

class TTable {
private:
    std::vector<TTEntry> table;
public:
    TTable() : table(hashSize) {};

    void Resize(U64 size) {
        table.resize(size);
    }

    void Clear() {
        table.clear();
        table.resize(hashSize);
    }

    TTEntry* GetRawEntry(U64 &hashKey) {
        TTEntry *current = &table[hashKey % hashSize];
        return current;
    }

    // returns used space per mill
    int GetUsedPercentage() {
        int count = 0;

        for (int i = 0; i < 1000; i++) {
            if (table[i]) count++;
        }

        return count;
    }

    SearchResults ReadEntry(U64 &hashKey, int depth, int alpha, int beta);

    void WriteEntry(U64 &hashKey, int depth, int score, int nodeType, Move besteMove);
};

inline TTable TT;
