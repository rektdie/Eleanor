#pragma once
#include <algorithm>
#include <iostream>

#define TUNING

#define AUTO_STEP(min_val, max_val) ((max_val - min_val) / 20.0)
#define AUTO_R_END_INT(min_val, max_val) \
    (0.002 / (std::min(0.5, static_cast<double>(AUTO_STEP(min_val, max_val))) / 0.5))
#define AUTO_R_END_FLOAT 0.002

#define TUNABLE_LIST \
    X_DOUBLE(lmrBaseQuiet, 1.1, 0.6, 1.6) \
    X_DOUBLE(lmrDivisorQuiet, 2.33, 1.6, 3.6) \
    X_DOUBLE(lmrBaseNoisy, 0.41, 0.1, 1.1) \
    X_DOUBLE(lmrDivisorNoisy, 3.20, 2.0, 4.5) \
    X_INT(lmrCutnode, 2048, 1024, 3072) \
    X_INT(lmrIsPV, 1024, 256, 2048) \
    X_INT(lmrTTPV, 1024, 256, 2048) \
    X_INT(lmrHistoryDivisor, 8192, 4096, 16384) \
    X_INT(lmrImproving, 1024, 256, 2048) \
    X_INT(lmrCorrplexity, 1024, 256, 2048) \
    X_INT(lmrTTPVFailLow, 1024, 256, 2048) \
    X_INT(lmrDeeperBase, 30, 0, 96) \
    X_INT(rfpBase, 25, 50, 200) \
    X_INT(rfpMargin, 100, 50, 200) \
    X_INT(fpMargin, 100, 50, 200) \
    X_INT(probcutBetaMargin, 200, 100, 300) \
    X_INT(razoringScalar, 200, 80, 320) \
    X_INT(doubleExtMargin, 30, 8, 64) \
    X_INT(seeQuietThreshold, -80, -160, 0) \
    X_INT(seeNoisyThreshold, -30, -120, 40) \
    X_INT(aspInitialDelta, 30, 8, 96) \
    X_INT(nmpBetaMargin, 30, 8, 96) \
    X_INT(seePawnValue, 100, 80, 140) \
    X_INT(seeKnightValue, 300, 240, 420) \
    X_INT(seeBishopValue, 350, 250, 450) \
    X_INT(seeRookValue, 500, 400, 700) \
    X_INT(seeQueenValue, 900, 700, 1300) \
    X_INT(seeQsThreshold, 0, -120, 120) \
    X_INT(seeOrderingThreshold, -100, -200, 0) \
    X_INT(ldseMargin, 25, 0, 96) \
    X_INT(historyBonusMultiplier, 300, 270, 330) \
    X_INT(historyBonusSub, 250, 230, 280) \
    X_INT(historyMalusMultiplier, 300, 270, 330) \
    X_INT(historyMalusSub, 250, 230, 280) \
    X_INT(contHistoryBonusMultiplier, 300, 270, 330) \
    X_INT(contHistoryBonusSub, 250, 230, 280) \
    X_INT(contHistoryMalusMultiplier, 300, 270, 330) \
    X_INT(contHistoryMalusSub, 250, 230, 280) \
    X_INT(captHistoryBonusMultiplier, 300, 270, 330) \
    X_INT(captHistoryBonusSub, 250, 230, 280) \
    X_INT(captHistoryMalusMultiplier, 300, 270, 330) \
    X_INT(captHistoryMalusSub, 250, 230, 280)

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
