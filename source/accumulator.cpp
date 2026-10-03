#include "accumulator.h"
#include "types.h"
#include "bitboard.h"

namespace ACC {

int CalculateIndex(bool perspective, bool side, int pieceType, int square, bool mirrored) {
    if (perspective == Black)
    {
        side   = 1 - side;
        square = square ^ 0b111000;
    }

    if (mirrored)
        square ^= 7;

    return side * 64 * 6 + pieceType * 64 + square;
}

const int16_t* Row(int bucket, int index) {
    return &NNUE::net.accumulator_weights[bucket][static_cast<size_t>(index) * NNUE::HL_SIZE];
}

void FinnyTable::Reset() {
    for (auto& perspective : entries) {
        for (auto& mirror : perspective) {
            for (auto& entry : mirror) {
                entry.acc = NNUE::net.accumulator_biases;
                for (auto& color : entry.bb)
                    for (auto& bb : color)
                        bb = 0;
            }
        }
    }
}

#if defined(_MSC_VER) && !defined(__clang__)
    #define ACC_NOINLINE __declspec(noinline)
#else
    #define ACC_NOINLINE __attribute__((noinline))
#endif

#if defined(__AVX512F__) && defined(__AVX512BW__)
    #include <immintrin.h>
    using Vec = __m512i;
    static inline Vec vLoad(const int16_t* p)       { return _mm512_loadu_si512(p); }
    static inline void vStore(int16_t* p, Vec v)    { _mm512_storeu_si512(p, v); }
    static inline Vec vAdd(Vec a, Vec b)            { return _mm512_add_epi16(a, b); }
    static inline Vec vSub(Vec a, Vec b)            { return _mm512_sub_epi16(a, b); }
    #define ACC_SIMD 1
#elif defined(__AVX2__)
    #include <immintrin.h>
    using Vec = __m256i;
    static inline Vec vLoad(const int16_t* p)       { return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p)); }
    static inline void vStore(int16_t* p, Vec v)    { _mm256_storeu_si256(reinterpret_cast<__m256i*>(p), v); }
    static inline Vec vAdd(Vec a, Vec b)            { return _mm256_add_epi16(a, b); }
    static inline Vec vSub(Vec a, Vec b)            { return _mm256_sub_epi16(a, b); }
    #define ACC_SIMD 1
#elif defined(__SSE2__) || defined(_M_X64) || defined(_M_AMD64)
    #include <immintrin.h>
    using Vec = __m128i;
    static inline Vec vLoad(const int16_t* p)       { return _mm_loadu_si128(reinterpret_cast<const __m128i*>(p)); }
    static inline void vStore(int16_t* p, Vec v)    { _mm_storeu_si128(reinterpret_cast<__m128i*>(p), v); }
    static inline Vec vAdd(Vec a, Vec b)            { return _mm_add_epi16(a, b); }
    static inline Vec vSub(Vec a, Vec b)            { return _mm_sub_epi16(a, b); }
    #define ACC_SIMD 1
#elif defined(__aarch64__)
    #include <arm_neon.h>
    using Vec = int16x8_t;
    static inline Vec vLoad(const int16_t* p)       { return vld1q_s16(p); }
    static inline void vStore(int16_t* p, Vec v)    { vst1q_s16(p, v); }
    static inline Vec vAdd(Vec a, Vec b)            { return vaddq_s16(a, b); }
    static inline Vec vSub(Vec a, Vec b)            { return vsubq_s16(a, b); }
    #define ACC_SIMD 1
#endif

#ifdef ACC_SIMD

constexpr size_t LANES = sizeof(Vec) / sizeof(int16_t);
constexpr size_t TILE = 4;
constexpr size_t STEP = LANES * TILE;
static_assert(NNUE::HL_SIZE % STEP == 0, "HL size must be divisible by the SIMD tile size");

template <int NA, int NS>
ACC_NOINLINE static void ApplyKernel(int16_t* dst, const int16_t* src,
                                     const int16_t* a1, const int16_t* a2,
                                     const int16_t* s1, const int16_t* s2) {
    for (size_t i = 0; i < NNUE::HL_SIZE; i += STEP) {
        Vec r[TILE];

        for (size_t u = 0; u < TILE; u++) r[u] = vLoad(src + i + u * LANES);
        for (size_t u = 0; u < TILE; u++) r[u] = vAdd(r[u], vLoad(a1 + i + u * LANES));
        if constexpr (NA > 1)
            for (size_t u = 0; u < TILE; u++) r[u] = vAdd(r[u], vLoad(a2 + i + u * LANES));
        for (size_t u = 0; u < TILE; u++) r[u] = vSub(r[u], vLoad(s1 + i + u * LANES));
        if constexpr (NS > 1)
            for (size_t u = 0; u < TILE; u++) r[u] = vSub(r[u], vLoad(s2 + i + u * LANES));

        for (size_t u = 0; u < TILE; u++) vStore(dst + i + u * LANES, r[u]);
    }
}

ACC_NOINLINE static void InPlaceKernel(int16_t* acc, const int16_t* const* adds, int nAdds,
                                       const int16_t* const* subs, int nSubs) {
    for (size_t i = 0; i < NNUE::HL_SIZE; i += STEP) {
        Vec r[TILE];

        for (size_t u = 0; u < TILE; u++) r[u] = vLoad(acc + i + u * LANES);

        for (int j = 0; j < nAdds; j++)
            for (size_t u = 0; u < TILE; u++) r[u] = vAdd(r[u], vLoad(adds[j] + i + u * LANES));

        for (int j = 0; j < nSubs; j++)
            for (size_t u = 0; u < TILE; u++) r[u] = vSub(r[u], vLoad(subs[j] + i + u * LANES));

        for (size_t u = 0; u < TILE; u++) vStore(acc + i + u * LANES, r[u]);
    }
}

void Apply(Accumulator& dst, const Accumulator& src, const int16_t* a1, const int16_t* a2, const int16_t* s1, const int16_t* s2) {
    if (a2)
        ApplyKernel<2, 2>(dst.data(), src.data(), a1, a2, s1, s2);
    else if (s2)
        ApplyKernel<1, 2>(dst.data(), src.data(), a1, nullptr, s1, s2);
    else
        ApplyKernel<1, 1>(dst.data(), src.data(), a1, nullptr, s1, nullptr);
}

void ApplyInPlace(Accumulator& acc, const int16_t* const* adds, int nAdds, const int16_t* const* subs, int nSubs) {
    InPlaceKernel(acc.data(), adds, nAdds, subs, nSubs);
}

void AddRow(Accumulator& acc, const int16_t* row) {
    const int16_t* rows[1] = {row};
    InPlaceKernel(acc.data(), rows, 1, nullptr, 0);
}

#else

using Ptr  = int16_t* __restrict;
using CPtr = const int16_t* __restrict;

ACC_NOINLINE static void Kernel1p1m(Ptr dst, CPtr src, CPtr a, CPtr s) {
    for (size_t i = 0; i < NNUE::HL_SIZE; i++)
        dst[i] = static_cast<int16_t>(src[i] + a[i] - s[i]);
}

ACC_NOINLINE static void Kernel1p2m(Ptr dst, CPtr src, CPtr a, CPtr s1, CPtr s2) {
    for (size_t i = 0; i < NNUE::HL_SIZE; i++)
        dst[i] = static_cast<int16_t>(src[i] + a[i] - s1[i] - s2[i]);
}

ACC_NOINLINE static void Kernel2p2m(Ptr dst, CPtr src, CPtr a1, CPtr a2, CPtr s1, CPtr s2) {
    for (size_t i = 0; i < NNUE::HL_SIZE; i++)
        dst[i] = static_cast<int16_t>(src[i] + a1[i] + a2[i] - s1[i] - s2[i]);
}

void Apply(Accumulator& dst, const Accumulator& src, const int16_t* a1, const int16_t* a2, const int16_t* s1, const int16_t* s2) {
    if (a2)
        Kernel2p2m(dst.data(), src.data(), a1, a2, s1, s2);
    else if (s2)
        Kernel1p2m(dst.data(), src.data(), a1, s1, s2);
    else
        Kernel1p1m(dst.data(), src.data(), a1, s1);
}

void ApplyInPlace(Accumulator& acc, const int16_t* const* adds, int nAdds, const int16_t* const* subs, int nSubs) {
    int16_t* p = acc.data();

    for (int j = 0; j < nAdds; j++)
        for (size_t i = 0; i < NNUE::HL_SIZE; i++)
            p[i] = static_cast<int16_t>(p[i] + adds[j][i]);

    for (int j = 0; j < nSubs; j++)
        for (size_t i = 0; i < NNUE::HL_SIZE; i++)
            p[i] = static_cast<int16_t>(p[i] - subs[j][i]);
}

void AddRow(Accumulator& acc, const int16_t* row) {
    const int16_t* rows[1] = {row};
    ApplyInPlace(acc, rows, 1, nullptr, 0);
}

#endif

}
