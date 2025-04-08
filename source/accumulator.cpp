#include "accumulator.h"
#include "types.h"

namespace ACC {

int CalculateIndex(bool perspective, int square, int pieceType, bool side) {
    if (perspective == Black)
    {
        side   = 1 - side;          // Flip side
        square = square ^ 0b111000; // Mirror square
    }
    
    return side * 64 * 6 + pieceType * 64 + square;
}

void Accumulator::Add(const NNUE::Network* net, int index) {
    for (int i = 0; i < NNUE::HL_SIZE; i ++) {
        values[i] += net->accumulator_weights[index][i];
    }
}

void Accumulator::Sub(const NNUE::Network* net, int index) {
    for (int i = 0; i < NNUE::HL_SIZE; i ++) {
        values[i] -= net->accumulator_weights[index][i];
    }
}


}
