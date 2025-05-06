#pragma once
#include <cstdint>
#include <string>
#include <array>

#include "../external/fmt/fmt/format.h"

class Board;

#ifdef __AVX512F__
constexpr usize ALIGNMENT = 64;
#else
constexpr size_t ALIGNMENT = 32;
#endif

namespace NNUE {

constexpr size_t INPUT_SIZE = 768;
constexpr size_t HL_SIZE = 128;

constexpr int16_t SCALE = 400;
constexpr int16_t QA = 255;
constexpr int16_t QB = 64;
    
struct Network {
    alignas(ALIGNMENT) std::array<int16_t, HL_SIZE * INPUT_SIZE> accumulator_weights;
    alignas(ALIGNMENT) std::array<int16_t, HL_SIZE> accumulator_biases;
    alignas(ALIGNMENT) std::array<int16_t, 2 * HL_SIZE> output_weights;
    int16_t output_bias;

    void Load(const std::string& path);

    int16_t Evaluate(const Board& board);
};

inline Network net;

}
