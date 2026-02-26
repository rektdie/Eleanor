#include "tt.h"

TTable SharedTT;

void TTable::WriteEntry(U64 &hashKey, int depth, int score, int nodeType, Move bestMove, bool ttpv) {
    TTBucket *bucket = &table[hashKey % table.size()];
    TTEntry *current = nullptr;

    if (bucket->depthPreferred && bucket->depthPreferred.hashKey == hashKey) {
        current = &bucket->depthPreferred;
    } else if (bucket->alwaysReplace && bucket->alwaysReplace.hashKey == hashKey) {
        current = &bucket->alwaysReplace;
    } else if (!bucket->alwaysReplace) {
        current = &bucket->alwaysReplace;
    } else if (!bucket->depthPreferred) {
        current = &bucket->depthPreferred;
    } else if (ttpv) {
        current = &bucket->depthPreferred;
    } else {
        const int depthPreferredValue = GetReplacementValue(bucket->depthPreferred);
        const int alwaysReplaceValue = GetReplacementValue(bucket->alwaysReplace);

        if (depthPreferredValue <= alwaysReplaceValue) {
            current = &bucket->depthPreferred;
        } else {
            current = &bucket->alwaysReplace;
        }
    }

    const bool samePosition = current->hashKey == hashKey;
    const bool exactBound = nodeType == PV;
    const bool entryFromCurrentAge = current->age == age;
    const int writeDepth = depth + 4 + ttpv * 2;

    if (samePosition && !exactBound && entryFromCurrentAge && writeDepth <= current->depth) {
        return;
    }

    current->hashKey = hashKey;
    current->nodeType = nodeType;
    current->score = score;
    current->depth = depth;
    
    if (bestMove || !samePosition) {
        current->bestMove = bestMove;
    }

    current->age = age;
    current->ttpv = ttpv;
}