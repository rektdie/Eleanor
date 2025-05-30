#include "board.h"
#include "movegen.h"
#include "uci.h"
#include "utils.h"
#include "benchmark.h"
#include "search.h"
#include "datagen.h"
#include "nnue.h"
#include "tests.h"

#ifndef EVALFILE
    #define EVALFILE "./nnue.bin"
#endif

#ifdef _MSC_VER
    #define MSVC
    #pragma push_macro("_MSC_VER")
    #undef _MSC_VER
#endif

#include "../external/incbin.h"

#ifdef MSVC
    #pragma pop_macro("_MSC_VER")
    #undef MSVC
#endif

#if !defined(_MSC_VER) || defined(__clang__)
INCBIN(EVAL, EVALFILE);
#endif

int main(int argc, char* argv[]) {
    auto loadDefaultNet = [&]([[maybe_unused]] bool warnMSVC = false) {
    #if defined(_MSC_VER) && !defined(__clang__)
            NNUE::net.Load(EVALFILE);
            if (warnMSVC)
                cerr << "WARNING: This file was compiled with MSVC, this means that an nnue was NOT embedded into the exe." << endl;
    #else
            NNUE::net = *reinterpret_cast<const NNUE::Network*>(gEVALData);
    #endif
        };
    
    loadDefaultNet(true);

        

	MOVEGEN::initLeaperAttacks();
	MOVEGEN::initSliderAttacks();
    UTILS::InitZobrist();
    SEARCH::InitLMRTable();

	Board board;

    if (argc > 1) {
        if (std::string(argv[1]) == "bench") {
            RunBenchmark();
        } else if (std::string(argv[1]) == "datagen") {
            int positions = 1;
            int threads = 1;
            if (argc > 2) {
                positions = std::stoi(argv[2]) * 1000;

                if (argc > 3) {
                    threads = std::stoi(argv[3]);
                }
            }
            DATAGEN::Run(positions, threads);
        }
    } else {
        UCILoop(board);
    }

	return 0;
}
