#include "tt.h"

void TTable::WriteEntry(U64 &hashKey, int depth, int score, int nodeType, Move bestMove, bool ttpv) {
    TTBucket *bucket = &table[hashKey % table.size()];
    TTEntry *current = nullptr;

    if (bucket->depthPreferred && bucket->depthPreferred.hashKey == hashKey) {
        current = &bucket->depthPreferred;
    } else if (bucket->alwaysReplace && bucket->alwaysReplace.hashKey == hashKey) {
        current = &bucket->alwaysReplace;
    } else if (ttpv) {
        current = &bucket->depthPreferred;
    } else if (!bucket->depthPreferred || bucket->depthPreferred.depth <= depth) {
        current = &bucket->depthPreferred;
    } else {
        current = &bucket->alwaysReplace;
    }

    current->hashKey = hashKey;
    current->nodeType = nodeType;
    current->score = score;
    current->depth = depth;
    current->bestMove = bestMove;
    current->ttpv = ttpv;
}
