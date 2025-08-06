#include "tt.h"

void TTable::WriteEntry(U64 &hashKey, int depth, int score, int nodeType, Move bestMove) {
    TTEntry *current = &table[hashKey % table.size()];

    current->hashKey = hashKey;
    current->nodeType = nodeType;
    current->score = score;
    current->depth = depth;
    current->bestMove = bestMove;
}
