#pragma once
#include "nnue.h"

namespace ACC {

using Accumulator = std::array<int16_t, NNUE::HL_SIZE>;

struct AccumulatorPair {
    alignas(32) Accumulator white;
    alignas(32) Accumulator black;
    
    bool mirroredWhite = false;
    bool mirroredBlack = false;

    // yoinked from Quinniboi - Prelude
    void addSub(bool stm, int add, int addPT, int sub, int subPT);
    void addSubSub(bool stm, int add, int addPT, int sub1, int subPT1, int sub2, int subPT2);
    void addAddSubSub(bool stm, int add1, int addPT1, int add2, int addPT2, int sub1, int subPT1, int sub2, int subPT2);
};

int CalculateIndex(bool perspective, bool side, int pieceType, int square, bool mirrored);

}
