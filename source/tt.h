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
    bool ttpv = false;

    TTEntry() {}

    TTEntry(int16_t score) {
        this->score = score;
    }

    operator U64() {
        return hashKey;
    }
};

class TTBucket {
public:
    TTEntry depthPreferred;
    TTEntry alwaysReplace;
};

// 1024 MB
constexpr U64 maxHash = (1024 * 1000000) / sizeof(TTBucket);
// 8 MB
constexpr U64 defaultHash = (8 * 1000000) / sizeof(TTBucket);

constexpr int16_t invalidEntry = 11111;

class TTable {
private:
    std::vector<TTBucket> table;
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

    void PrefetchEntry(const U64 &hashKey) {
        __builtin_prefetch(&table[hashKey % table.size()]);
    }

    TTEntry GetEntry(const U64 &hashKey) {
        TTBucket *current = &table[hashKey % table.size()];
        if (current->depthPreferred.hashKey == hashKey) {
            return current->depthPreferred;
        } else if (current->alwaysReplace.hashKey == hashKey) {
            return current->alwaysReplace;
        }

        return TTEntry();
    }

    // returns used space per mill
    int GetUsedPercentage() {
        int count = 0;
        const U64 sampleSize = std::min<U64>(1000, table.size());

        for (U64 i = 0; i < sampleSize; i++) {
            if (table[i].depthPreferred || table[i].alwaysReplace) count++;
        }

        return count;
    }

    void WriteEntry(const U64 &hashKey, int depth, int score, int nodeType, Move bestMove, bool ttpv);
};

extern TTable SharedTT;
