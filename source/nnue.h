#pragma once
#include <cstdint>
#include <string>

namespace NNUE {

constexpr int INPUT_SIZE = 768;
constexpr int HL_SIZE = 64;

constexpr int SCALE = 400;
constexpr int QA = 255;
constexpr int QB = 64;
    
struct Network {
    int16_t accumulator_weights[INPUT_SIZE][HL_SIZE];
    int16_t accumulator_biases[HL_SIZE];
    int16_t output_weights[2 * HL_SIZE];
    int16_t output_bias;

    void Load(const std::string& path);
};

}
