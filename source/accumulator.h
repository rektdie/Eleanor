#pragma once
#include "nnue.h"

namespace ACC {

struct Accumulator {
    std::array<int16_t, NNUE::HL_SIZE> values;

    void Add(const NNUE::Network* net, int index);
    void Sub(const NNUE::Network* net, int index);
};

struct AccumulatorPair {
    Accumulator white;
    Accumulator black;
};

int CalculateIndex(bool perspective, int square, int pieceType, bool side);

}
