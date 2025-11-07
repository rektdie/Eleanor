#include "accumulator.h"
#include "types.h"
#include "bitboard.h"

namespace ACC {

int CalculateIndex(bool perspective, bool side, int pieceType, int square, bool mirrored) {
    if (perspective == Black)
    {
        side   = 1 - side;          // Flip side
        square = square ^ 0b111000; // Mirror square
    }

    if (mirrored)
        square ^= 7;
    
    return side * 64 * 6 + pieceType * 64 + square;
}

// All friendly, for quiets
void AccumulatorPair::addSub(bool stm, int add, int addPT, int sub, int subPT) {
    int addW = CalculateIndex(White, stm, addPT, add, mirroredWhite);
    int addB = CalculateIndex(Black, stm, addPT, add, mirroredBlack);

    int subW = CalculateIndex(White, stm, subPT, sub, mirroredWhite);
    int subB = CalculateIndex(Black, stm, subPT, sub, mirroredBlack);

    for (int i = 0; i < NNUE::HL_SIZE; i++) {
        white[i] += NNUE::net.accumulator_weights[addW * NNUE::HL_SIZE + i]
        - NNUE::net.accumulator_weights[subW * NNUE::HL_SIZE + i];
        
        black[i] += NNUE::net.accumulator_weights[addB * NNUE::HL_SIZE + i]
        - NNUE::net.accumulator_weights[subB * NNUE::HL_SIZE + i];
    }
}

// Captures
void AccumulatorPair::addSubSub(bool stm, int add, int addPT, int sub1, int subPT1, int sub2, int subPT2) {
    int addW = CalculateIndex(White, stm, addPT, add, mirroredWhite);
    int addB = CalculateIndex(Black, stm, addPT, add, mirroredBlack);

    int subW1 = CalculateIndex(White, stm, subPT1, sub1, mirroredWhite);
    int subB1 = CalculateIndex(Black, stm, subPT1, sub1, mirroredBlack);

    int subW2 = CalculateIndex(White, !stm, subPT2, sub2, mirroredWhite);
    int subB2 = CalculateIndex(Black, !stm, subPT2, sub2, mirroredBlack);

    for (int i = 0; i < NNUE::HL_SIZE; i++) {
        white[i] += NNUE::net.accumulator_weights[addW * NNUE::HL_SIZE + i]
        - NNUE::net.accumulator_weights[subW1 * NNUE::HL_SIZE + i]
        - NNUE::net.accumulator_weights[subW2 * NNUE::HL_SIZE + i];

        black[i] += NNUE::net.accumulator_weights[addB * NNUE::HL_SIZE + i]
        - NNUE::net.accumulator_weights[subB1 * NNUE::HL_SIZE + i]
        - NNUE::net.accumulator_weights[subB2 * NNUE::HL_SIZE + i];
    }
}

// Castling
void AccumulatorPair::addAddSubSub(bool stm, int add1, int addPT1, int add2, int addPT2, int sub1, int subPT1, int sub2, int subPT2) {
    int addW1 = CalculateIndex(White, stm, addPT1, add1, mirroredWhite);
    int addB1 = CalculateIndex(Black, stm, addPT1, add1, mirroredBlack);

    int addW2 = CalculateIndex(White, stm, addPT2, add2, mirroredWhite);
    int addB2 = CalculateIndex(Black, stm, addPT2, add2, mirroredBlack);

    int subW1 = CalculateIndex(White, stm, subPT1, sub1, mirroredWhite);
    int subB1 = CalculateIndex(Black, stm, subPT1, sub1, mirroredBlack);

    int subW2 = CalculateIndex(White, stm, subPT2, sub2, mirroredWhite);
    int subB2 = CalculateIndex(Black, stm, subPT2, sub2, mirroredBlack);

    for (int i = 0; i < NNUE::HL_SIZE; i++) {
        white[i] += NNUE::net.accumulator_weights[addW1 * NNUE::HL_SIZE + i]
        + NNUE::net.accumulator_weights[addW2 * NNUE::HL_SIZE + i]
        - NNUE::net.accumulator_weights[subW1 * NNUE::HL_SIZE + i]
        - NNUE::net.accumulator_weights[subW2 * NNUE::HL_SIZE + i];

        black[i] += NNUE::net.accumulator_weights[addB1 * NNUE::HL_SIZE + i]
        + NNUE::net.accumulator_weights[addB2 * NNUE::HL_SIZE + i]
        - NNUE::net.accumulator_weights[subB1 * NNUE::HL_SIZE + i]
        - NNUE::net.accumulator_weights[subB2 * NNUE::HL_SIZE + i];
    }
}

}
