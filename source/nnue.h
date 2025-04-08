#pragma once
#include <cstdint>
#include <string>
#include <array>

namespace NNUE {

constexpr int INPUT_SIZE = 768;
constexpr int HL_SIZE = 64;

constexpr int SCALE = 400;
constexpr int QA = 255;
constexpr int QB = 64;
    
struct Network {
    std::array<int16_t, HL_SIZE * INPUT_SIZE> accumulator_weights;
    std::array<int16_t, HL_SIZE> accumulator_biases;
    std::array<int16_t, 2 * HL_SIZE> output_weights;
    int16_t output_bias;

    void Load(const std::string& path);
};

inline Network net;

}
