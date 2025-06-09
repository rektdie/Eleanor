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
    int16_t score = 0;
    Move bestMove = Move();
    uint8_t depth = 0;
    uint8_t nodeType = 0;

    TTEntry() {}

    TTEntry(int16_t score) {
        this->score = score;
    }

    operator U64() {
        return hashKey;
    }
};

// 1024 MB
constexpr U64 maxHash = (1024 * 1000000) / sizeof(TTEntry);
// 8 MB
constexpr U64 defaultHash = (8 * 1000000) / sizeof(TTEntry);

constexpr int16_t invalidEntry = 111111;

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

    TTEntry ReadEntry(U64 &hashKey, int depth, int alpha, int beta);

    void WriteEntry(U64 &hashKey, int depth, int score, int nodeType, Move besteMove);
};
