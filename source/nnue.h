#pragma once
#include <cstdint>
#include <string>
#include <array>

#include "../external/fmt/fmt/format.h"

class Board;

#ifdef __AVX512F__
constexpr size_t ALIGNMENT = 64;
#else
constexpr size_t ALIGNMENT = 32;
#endif

namespace NNUE {

constexpr size_t INPUT_SIZE = 768;
constexpr size_t HL_SIZE = 1536;
constexpr size_t OUTPUT_BUCKETS = 8;

constexpr int16_t SCALE = 400;
constexpr int16_t QA = 255;
constexpr int16_t QB = 64;
    
struct Network {
    alignas(ALIGNMENT) std::array<int16_t, HL_SIZE * INPUT_SIZE> accumulator_weights;
    alignas(ALIGNMENT) std::array<int16_t, HL_SIZE> accumulator_biases;
    alignas(ALIGNMENT) std::array<std::array<int16_t, 2 * HL_SIZE>, OUTPUT_BUCKETS> output_weights;
    std::array<int16_t, OUTPUT_BUCKETS> output_bias;

    void Load(const std::string& path);

    int16_t Evaluate(const Board& board);
};

inline Network net;

}
