#pragma once
#include <iostream>

//#define TUNING

#define AUTO_STEP(min_val, max_val) ((max_val - min_val) / 20.0)

#define TUNABLE_LIST \
    X_DOUBLE(lmrBaseQuiet, 1.1, 0.5, 1.5) \
    X_DOUBLE(lmrDivisorQuiet, 2.33, 1.5, 3.5) \
    X_DOUBLE(lmrBaseNoisy, 0.41, 0.5, 1.5) \
    X_DOUBLE(lmrDivisorNoisy, 3.20, 1.5, 3.5) \
    X_INT(lmrCutnode, 2048, 1024, 3072) \
    X_INT(lmrIsPV, 1024, 512, 2048) \
    X_INT(lmrTTPV, 1024, 512, 2048) \
    X_INT(lmrHistoryDivisor, 8192, 6000, 10000) \
    X_INT(lmrImproving, 1024, 6000, 10000) \
    X_INT(lmrCorrplexity, 1024, 6000, 10000) \
    X_INT(rfpBase, 25, 50, 200) \
    X_INT(rfpMargin, 100, 50, 200) \
    X_INT(fpMargin, 100, 50, 200) \
    X_INT(razoringScalar, 200, 50, 200) \
    X_INT(doubleExtMargin, 30, 10, 60) \
    X_INT(seeQuietThreshold, -80, -150, -20) \
    X_INT(seeNoisyThreshold, -30, -80, 20) \
    X_INT(aspInitialDelta, 30, 20, 80) \
    X_INT(aspSwingSqDiv, 24, 20, 80) \
    X_INT(aspMaxExtra, 400, 20, 80) \
    X_INT(nmpBetaMargin, 30, 20, 80) \
    X_INT(seePawnValue, 100, 80, 120) \
    X_INT(seeKnightValue, 300, 250, 350) \
    X_INT(seeBishopValue, 350, 300, 400) \
    X_INT(seeRookValue, 500, 400, 600) \
    X_INT(seeQueenValue, 900, 750, 1100) \
    X_INT(seeQsThreshold, 0, -50, 50) \
    X_INT(seeOrderingThreshold, -100, -120, -80) \
    X_INT(historyBonusMultiplier, 300, 270, 330) \
    X_INT(historyBonusSub, 250, 230, 280)

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
                  << ", 0.002" << std::endl;

    #define X_INT(name, default_val, min_val, max_val) \
        std::cout << #name << ", int, " << name \
                  << ", " << min_val \
                  << ", " << max_val \
                  << ", " << name##_step \
                  << ", 0.002" << std::endl;

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
