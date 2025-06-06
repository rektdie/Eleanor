#include "tt.h"

TTEntry TTable::ReadEntry(U64 &hashKey, int depth, int alpha, int beta) {
    TTEntry *current = &table[hashKey % table.size()];

    if (current->hashKey == hashKey) {
        if (current->depth >= depth) {
            if (current->nodeType == PV){
                return *current;
            } else if (current->nodeType == AllNode && current->score <= alpha) {
                return *current;
            } else if (current->nodeType == CutNode && current->score >= beta) {
                return *current;
            }
        }
    }

    // returning invalid value
    return invalidEntry;
}

void TTable::WriteEntry(U64 &hashKey, int depth, int score, int nodeType, Move bestMove) {
    TTEntry *current = &table[hashKey % table.size()];

    current->hashKey = hashKey;
    current->nodeType = nodeType;
    current->score = score;
    current->depth = depth;
    current->bestMove = bestMove;
}
