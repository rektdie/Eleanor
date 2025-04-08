#include <immintrin.h>
#include "nnue.h"
#include "accumulator.h"
#include "board.h"
#include "types.h"

namespace NNUE {

static int32_t Forward(const Board& board,
                       const Network* net) {
    const ACC::Accumulator& stmAcc = board.sideToMove == White ? board.accPair.white : board.accPair.black;
    const ACC::Accumulator& nstmAcc = !board.sideToMove == White ? board.accPair.white : board.accPair.black;

    int32_t eval = 0;

    const __m256i vec_zero = _mm256_setzero_si256();
    const __m256i vec_qa   = _mm256_set1_epi16(QA);
    __m256i       sum      = vec_zero;

    for (int i = 0; i < HL_SIZE / 16; i++) {
        const __m256i us   = _mm256_load_si256(reinterpret_cast<const __m256i*>(&stmAcc.values[16 * i]));  // Load from accumulator
        const __m256i them = _mm256_load_si256(reinterpret_cast<const __m256i*>(&nstmAcc.values[16 * i]));

        const __m256i us_weights   = _mm256_load_si256(reinterpret_cast<const __m256i*>(&net->output_weights[16 * i]));  // Load from net
        const __m256i them_weights = _mm256_load_si256(reinterpret_cast<const __m256i*>(&net->output_weights[HL_SIZE + 16 * i]));

        const __m256i us_clamped   = _mm256_min_epi16(_mm256_max_epi16(us, vec_zero), vec_qa);
        const __m256i them_clamped = _mm256_min_epi16(_mm256_max_epi16(them, vec_zero), vec_qa);

        const __m256i us_results   = _mm256_madd_epi16(_mm256_mullo_epi16(us_weights, us_clamped), us_clamped);
        const __m256i them_results = _mm256_madd_epi16(_mm256_mullo_epi16(them_weights, them_clamped), them_clamped);

        sum = _mm256_add_epi32(sum, us_results);
        sum = _mm256_add_epi32(sum, them_results);
    }

    __m256i v1 = _mm256_hadd_epi32(sum, sum);
    __m256i v2 = _mm256_hadd_epi32(v1, v1);

    eval = _mm256_extract_epi32(v2, 0) + _mm256_extract_epi32(v2, 4);

    eval /= QA;

    eval += net->output_bias;

    return (eval * SCALE) / (QA * QB);
}
    
}
