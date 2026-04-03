#pragma once
#include <algorithm>
#include <iostream>

#define TUNING

#define AUTO_STEP(min_val, max_val) ((max_val - min_val) / 20.0)
#define AUTO_R_END_INT(min_val, max_val) \
    (0.002 / (std::min(0.5, static_cast<double>(AUTO_STEP(min_val, max_val))) / 0.5))
#define AUTO_R_END_FLOAT 0.002

#define TUNABLE_LIST \
    X_DOUBLE(lmrBaseQuiet, 1.1029723689888005, 0.6, 1.6) \
    X_DOUBLE(lmrDivisorQuiet, 2.3063513397139945, 1.6, 3.6) \
    X_DOUBLE(lmrBaseNoisy, 0.451805556082857, 0.1, 1.1) \
    X_DOUBLE(lmrDivisorNoisy, 3.1355330830404338, 2.0, 4.5) \
    X_INT(lmrCutnode, 2005, 1024, 3072) \
    X_INT(lmrIsPV, 982, 256, 2048) \
    X_INT(lmrTTPV, 936, 256, 2048) \
    X_INT(lmrHistoryDivisor, 8377, 4096, 16384) \
    X_INT(lmrImproving, 1131, 256, 2048) \
    X_INT(lmrCorrplexity, 830, 256, 2048) \
    X_INT(lmrTTPVFailLow, 913, 256, 2048) \
    X_INT(lmrDeeperBase, 46, 0, 96) \
    X_INT(rfpBase, 21, 0, 100) \
    X_INT(rfpMargin, 97, 50, 200) \
    X_INT(fpMargin, 86, 50, 200) \
    X_INT(probcutBetaMargin, 190, 100, 300) \
    X_INT(razoringScalar, 195, 80, 320) \
    X_INT(doubleExtMargin, 32, 8, 64) \
    X_INT(seeQuietThreshold, -81, -160, 0) \
    X_INT(seeNoisyThreshold, -32, -120, 40) \
    X_INT(aspInitialDelta, 26, 8, 96) \
    X_INT(nmpBetaMargin, 25, 8, 96) \
    X_INT(seePawnValue, 98, 80, 140) \
    X_INT(seeKnightValue, 301, 240, 420) \
    X_INT(seeBishopValue, 346, 250, 450) \
    X_INT(seeRookValue, 507, 400, 700) \
    X_INT(seeQueenValue, 924, 700, 1300) \
    X_INT(seeQsThreshold, -13, -120, 120) \
    X_INT(seeOrderingThreshold, -102, -200, 0) \
    X_INT(ldseMargin, 26, 0, 96) \
    X_INT(historyBonusMultiplier, 306, 280, 320) \
    X_INT(historyBonusSub, 252, 235, 265) \
    X_INT(historyMalusMultiplier, 301, 280, 320) \
    X_INT(historyMalusSub, 250, 235, 265) \
    X_INT(contHistoryBonusMultiplier, 300, 280, 320) \
    X_INT(contHistoryBonusSub, 252, 235, 265) \
    X_INT(contHistoryMalusMultiplier, 300, 280, 320) \
    X_INT(contHistoryMalusSub, 249, 235, 265) \
    X_INT(captHistoryBonusMultiplier, 302, 280, 320) \
    X_INT(captHistoryBonusSub, 249, 235, 265) \
    X_INT(captHistoryMalusMultiplier, 300, 280, 320) \
    X_INT(captHistoryMalusSub, 249, 235, 265)

#ifdef TUNING
    #define X_DOUBLE(name, default_val, min_val, max_val) \
        inline double name = default_val; \
        constexpr double name##_step = AUTO_STEP(min_val, max_val);
    #define X_INT(name, default_val, min_val, max_val) \
        inline int name = default_val; \
        constexpr double name##_step = AUTO_STEP(min_val, max_val);
#else
    #define X_DOUBLE(name, default_val, min_val, max_val) \
        constexpr double name = default_val; \
        constexpr double name##_step = AUTO_STEP(min_val, max_val);
    #define X_INT(name, default_val, min_val, max_val) \
        constexpr int name = default_val; \
        constexpr double name##_step = AUTO_STEP(min_val, max_val);
#endif

TUNABLE_LIST

#undef X_DOUBLE
#undef X_INT

inline void PrintTunables() {
    #define X_DOUBLE(name, default_val, min_val, max_val) \
        std::cout << #name << ", float, " << name \
                  << ", " << min_val \
                  << ", " << max_val \
                  << ", " << name##_step \
                  << ", " << AUTO_R_END_FLOAT << std::endl;

    #define X_INT(name, default_val, min_val, max_val) \
        std::cout << #name << ", int, " << name \
                  << ", " << min_val \
                  << ", " << max_val \
                  << ", " << name##_step \
                  << ", " << AUTO_R_END_INT(min_val, max_val) << std::endl;

    TUNABLE_LIST

    #undef X_DOUBLE
    #undef X_INT
}

inline void PrintTunablesUCI() {
    #define X_DOUBLE(name, default_val, min_val, max_val) \
        std::cout << "option name " #name \
                  << " type string default " << name \
                  << " min " << min_val \
                  << " max " << max_val \
                  << std::endl;

    #define X_INT(name, default_val, min_val, max_val) \
        std::cout << "option name " #name \
                  << " type spin default " << name \
                  << " min " << min_val \
                  << " max " << max_val \
                  << std::endl;

    TUNABLE_LIST

    #undef X_DOUBLE
    #undef X_INT
}
