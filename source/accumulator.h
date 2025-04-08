#pragma once
#include "nnue.h"

namespace ACC {

struct Accumulator {
    int16_t values[NNUE::HL_SIZE];

    void Add(const NNUE::Network* net, int index);
    void Sub(const NNUE::Network* net, int index);
};

struct AccumulatorPair {
    Accumulator white;
    Accumulator black;
};

int CalculateIndex(bool perspective, int square, int pieceType, bool side);

}
