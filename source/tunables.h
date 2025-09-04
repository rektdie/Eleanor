#pragma once
#include <iostream>

#define TUNING

#define TUNABLE_LIST \
    X_DOUBLE(lmrBase, 0.77, 0.5, 1.5, 0.01) \
    X_DOUBLE(lmrDivisor, 2.36, 1.5, 4.0, 0.01) \
    X_INT(lmrDepth, 3, 1, 6, 1) \
    X_INT(lmrMoveSeen, 2, 1, 5, 1) \
    X_INT(lmrMoveSeenPVModifier, 2, 1, 5, 1) \
    X_INT(rfpMargin, 100, 50, 200, 5) \
    X_INT(rfpDepth, 7, 4, 12, 1) \
    X_INT(nmpDepth, 1, 1, 4, 1) \
    X_INT(nmpReduction, 4, 2, 6, 1) \
    X_INT(lmpBase, 7, 3, 15, 1) \
    X_INT(lmpMultiplier, 4, 2, 8, 1) \
    X_INT(fpMargin, 100, 50, 200, 5) \
    X_INT(fpDepth, 5, 3, 8, 1) \
    X_INT(doubleExtMargin, 30, 10, 60, 5) \
    X_INT(seeQuietThreshold, -80, -150, -20, 5) \
    X_INT(seeNoisyThreshold, -30, -80, 10, 5) \
    X_INT(seeDepth, 10, 6, 15, 1) \
    X_INT(aspInitialDelta, 50, 20, 100, 5) \
    X_INT(seePawnValue, 100, 80, 120, 5) \
    X_INT(seeKnightValue, 300, 250, 350, 10) \
    X_INT(seeBishopValue, 350, 300, 400, 10) \
    X_INT(seeRookValue, 500, 450, 550, 10) \
    X_INT(seeQueenValue, 900, 800, 1000, 25)

#ifdef TUNING
    #define X_DOUBLE(name, default_val, min_val, max_val, step_val) \
        inline double name = default_val;
    #define X_INT(name, default_val, min_val, max_val, step_val) \
        inline int name = default_val;
#else
    #define X_DOUBLE(name, default_val, min_val, max_val, step_val) \
        constexpr double name = default_val;
    #define X_INT(name, default_val, min_val, max_val, step_val) \
        constexpr int name = default_val;
#endif

TUNABLE_LIST

#undef X_DOUBLE
#undef X_INT

inline void PrintTunables() {
    #define X_DOUBLE(name, default_val, min_val, max_val, step_val) \
        std::cout << #name << ", float, " << name \
                  << ", " << min_val \
                  << ", " << max_val \
                  << ", " << step_val \
                  << ", 0.002" << std::endl;

    #define X_INT(name, default_val, min_val, max_val, step_val) \
        std::cout << #name << ", int, " << name \
                  << ", " << min_val \
                  << ", " << max_val \
                  << ", " << step_val \
                  << ", 0.002" << std::endl;

    TUNABLE_LIST

    #undef X_DOUBLE
    #undef X_INT
}

inline void PrintTunablesUCI() {
    #define X_DOUBLE(name, default_val, min_val, max_val, step_val) \
        std::cout << "option name " #name \
                  << " type string default " << name \
                  << " min " << min_val \
                  << " max " << max_val \
                  << std::endl;

    #define X_INT(name, default_val, min_val, max_val, step_val) \
        std::cout << "option name " #name \
                  << " type spin default " << name \
                  << " min " << min_val \
                  << " max " << max_val \
                  << std::endl;

    TUNABLE_LIST

    #undef X_DOUBLE
    #undef X_INT
}


