#include <fstream>
#include <algorithm>

#include "nnue.h"
#include "accumulator.h"
#include "board.h"
#include "types.h"
#include "search.h"

namespace NNUE {

void Network::Load(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()){
        std::cerr << "Failed to open file " + path << std::endl;
    }

    // HL weights
    for (size_t i = 0; i < accumulator_weights.size(); i++) {
        file.read(reinterpret_cast<char*>(&accumulator_weights[i]), sizeof(int16_t));
    }

    // HL biases
    for (size_t i = 0; i < accumulator_biases.size(); i++) {
        file.read(reinterpret_cast<char*>(&accumulator_biases[i]), sizeof(int16_t));
    }

    // Output weights
    for (size_t i = 0; i < output_weights.size(); i++) {
        for (size_t j = 0; j < output_weights[i].size(); j++) {
            file.read(reinterpret_cast<char*>(&output_weights[i][j]), sizeof(int16_t));
        }
    }

    // Output biases
    for (size_t i = 0; i < output_bias.size(); i++) {
        file.read(reinterpret_cast<char*>(&output_bias[i]), sizeof(int16_t));
    }
}

#if defined(__x86_64__) || defined(__amd64__) || (defined(_WIN64) && (defined(_M_X64) || defined(_M_AMD64)))
#include <immintrin.h>
#if defined(__AVX512F__)
#pragma message("Using AVX512 NNUE inference")
using nativeVector = __m512i;
#define set1_epi16 _mm512_set1_epi16
#define load_epi16 _mm512_load_si512
#define min_epi16 _mm512_min_epi16
#define max_epi16 _mm512_max_epi16
#define madd_epi16 _mm512_madd_epi16
#define mullo_epi16 _mm512_mullo_epi16
#define add_epi32 _mm512_add_epi32
#define reduce_epi32 _mm512_reduce_add_epi32
#elif defined(__AVX2__)
#pragma message("Using AVX2 NNUE inference")
using nativeVector = __m256i;
#define set1_epi16 _mm256_set1_epi16
#define load_epi16 _mm256_load_si256
#define min_epi16 _mm256_min_epi16
#define max_epi16 _mm256_max_epi16
#define madd_epi16 _mm256_madd_epi16
#define mullo_epi16 _mm256_mullo_epi16
#define add_epi32 _mm256_add_epi32
#define reduce_epi32 \
    [](nativeVector vec) { \
        __m128i xmm1 = _mm256_extracti128_si256(vec, 1); \
        __m128i xmm0 = _mm256_castsi256_si128(vec); \
        xmm0         = _mm_add_epi32(xmm0, xmm1); \
        xmm1         = _mm_shuffle_epi32(xmm0, 238); \
        xmm0         = _mm_add_epi32(xmm0, xmm1); \
        xmm1         = _mm_shuffle_epi32(xmm0, 85); \
        xmm0         = _mm_add_epi32(xmm0, xmm1); \
        return _mm_cvtsi128_si32(xmm0); \
    }
#else
#pragma message("Using SSE NNUE inference")
using nativeVector = __m128i;
#define set1_epi16 _mm_set1_epi16
#define load_epi16 _mm_load_si128
#define min_epi16 _mm_min_epi16
#define max_epi16 _mm_max_epi16
#define madd_epi16 _mm_madd_epi16
#define mullo_epi16 _mm_mullo_epi16
#define add_epi32 _mm_add_epi32
#define reduce_epi32 \
    [](nativeVector vec) { \
        __m128i xmm1 = _mm_shuffle_epi32(vec, 238); \
        vec          = _mm_add_epi32(vec, xmm1); \
        xmm1         = _mm_shuffle_epi32(vec, 85); \
        vec          = _mm_add_epi32(vec, xmm1); \
        return _mm_cvtsi128_si32(vec); \
    }
#endif

#elif defined(__aarch64__)
#pragma message("Using ARM64 (NEON) NNUE inference")
#include <arm_neon.h>
using nativeVector = int16x8_t;
static inline nativeVector set1_epi16(int16_t v) {
    return vdupq_n_s16(v);
}
static inline nativeVector load_epi16(const nativeVector* ptr) {
    return *ptr;
}
static inline nativeVector min_epi16(nativeVector a, nativeVector b) {
    return vminq_s16(a, b);
}
static inline nativeVector max_epi16(nativeVector a, nativeVector b) {
    return vmaxq_s16(a, b);
}
static inline int32x4_t madd_epi16(nativeVector a, nativeVector b) {
    int32x4_t lo = vmull_s16(vget_low_s16(a), vget_low_s16(b));
    int32x4_t hi = vmull_s16(vget_high_s16(a), vget_high_s16(b));
    int32x4_t sum_lo = vpaddq_s32(lo, lo);
    int32x4_t sum_hi = vpaddq_s32(hi, hi);
    return vcombine_s32(vget_low_s32(sum_lo), vget_low_s32(sum_hi));
}
static inline nativeVector mullo_epi16(nativeVector a, nativeVector b) {
    return vmulq_s16(a, b);
}
static inline int32x4_t add_epi32(int32x4_t a, int32x4_t b) {
    return vaddq_s32(a, b);
}
static inline int reduce_epi32(int32x4_t v) {
    int32x2_t pair = vadd_s32(vget_low_s32(v), vget_high_s32(v));
    int32x2_t total = vpadd_s32(pair, pair);
    return vget_lane_s32(total, 0);
}

#else
#pragma message("Using fallback scalar NNUE inference")
using nativeVector = void*;
#endif


static int32_t VectorizedSCReLU(const Board& board, const Network& net, size_t outputBucket) {
    const size_t VECTOR_SIZE = sizeof(nativeVector) / sizeof(int16_t);
    static_assert(HL_SIZE % VECTOR_SIZE == 0, "HL size must be divisible by the native register size of your CPU for vectorization to work");

    const ACC::Accumulator& stmAcc = board.sideToMove == White ? board.accPair.white : board.accPair.black;
    const ACC::Accumulator& nstmAcc = !board.sideToMove == White ? board.accPair.white : board.accPair.black;

    const nativeVector VEC_QA   = set1_epi16(QA);
    const nativeVector VEC_ZERO = set1_epi16(0);

    nativeVector accumulator{};

    for (int i = 0; i < HL_SIZE; i += VECTOR_SIZE) {
        // Load accumulators
        const nativeVector stmAccumValues   = load_epi16(reinterpret_cast<const nativeVector*>(&stmAcc[i]));
        const nativeVector nstmAccumValues  = load_epi16(reinterpret_cast<const nativeVector*>(&nstmAcc[i]));

        // Clamp values
        const nativeVector stmClamped   = min_epi16(VEC_QA, max_epi16(stmAccumValues, VEC_ZERO));
        const nativeVector nstmClamped  = min_epi16(VEC_QA, max_epi16(nstmAccumValues, VEC_ZERO));

        // Load weights
        const nativeVector stmWeights   = load_epi16(reinterpret_cast<const nativeVector*>(&net.output_weights[outputBucket][i]));
        const nativeVector nstmWeights  = load_epi16(reinterpret_cast<const nativeVector*>(&net.output_weights[outputBucket][i + HL_SIZE]));

        // SCReLU activation
        const nativeVector stmActivated  = madd_epi16(stmClamped, mullo_epi16(stmClamped, stmWeights));
        const nativeVector nstmActivated  = madd_epi16(nstmClamped, mullo_epi16(nstmClamped, nstmWeights));

        accumulator = add_epi32(accumulator, stmActivated);
        accumulator = add_epi32(accumulator, nstmActivated);
    }

    return reduce_epi32(accumulator);
}

int Forward(const Board& board, const Network& net) {
    bool stm = board.sideToMove;

    const size_t divisor      = 32 / OUTPUT_BUCKETS;
    const size_t outputBucket = (board.occupied.PopCount() - 2) / divisor;

    const ACC::Accumulator& stmACC = stm ? board.accPair.white : board.accPair.black;
    const ACC::Accumulator& nstmACC = !stm ? board.accPair.white : board.accPair.black;

    int64_t eval = 0;

    eval = VectorizedSCReLU(board, net, outputBucket);

    eval /= QA;

    eval += net.output_bias[outputBucket];git show --summary


    return (eval * SCALE) / (QA * QB);
}

int16_t Network::Evaluate(const Board& board) {
    return std::clamp(Forward(board, *this), (-SEARCH::MATE_SCORE + SEARCH::MAX_PLY),
        (SEARCH::MATE_SCORE - SEARCH::MAX_PLY));
}

}
