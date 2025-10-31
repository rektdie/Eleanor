#include "tt.h"

void TTable::WriteEntry(U64 &hashKey, int depth, int score, int nodeType, Move bestMove) {
    TTBucket *bucket = &table[hashKey % table.size()];
    TTEntry *current;

    if (bucket->depthPreferred.depth <= depth) {
        current = &bucket->depthPreferred;
    } else {
        current = &bucket->alwaysReplace;
    }

    current->hashKey = hashKey;
    current->nodeType = nodeType;
    current->score = score;
    current->depth = depth;
    current->bestMove = bestMove;
}
