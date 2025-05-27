#pragma once
#include "board.h"
#include <vector>

class SearchResults {
public:
    int score = 0;
    Move bestMove = Move();

    SearchResults(){}

    SearchResults(int pScore, Move pMove) {
        score = pScore;
        bestMove = pMove;
    }

    SearchResults(int pScore) {
        score = pScore;
    }
};

class TTEntry {
public:
    U64 hashKey = 0;
    int nodeType = 0;
    uint16_t score = 0;
    Move bestMove = Move();
    uint8_t depth = 0;

    operator U64() {
        return hashKey;
    }
};

// 64 MB
constexpr U64 maxHash = (64* 1000000) / sizeof(TTEntry);
// 5 MB
constexpr U64 defaultHash = (5 * 1000000) / sizeof(TTEntry);

constexpr int invalidEntry = 111111;

class TTable {
private:
    std::vector<TTEntry> table;
public:
    TTable() : table(defaultHash) {};

    void Resize(U64 size) {
        table.resize(size);
    }

    void Clear() {
        U64 size = table.size();
        table.clear();
        table.resize(size);
    }

    TTEntry* GetRawEntry(U64 &hashKey) {
        TTEntry *current = &table[hashKey % table.size()];
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
