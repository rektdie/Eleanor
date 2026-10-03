#include "board.h"
#include "movegen.h"
#include "uci.h"
#include "utils.h"
#include "benchmark.h"
#include "search.h"
#include "datagen.h"
#include "datagenui.h"
#include "nnue.h"
#include "tests.h"

#include <iostream>
#include <string>

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

// Strict integer parsing: returns 0 when the text is not a positive whole number,
// so bad CLI input reports a usage error instead of throwing std::invalid_argument.
static int ParsePositiveInt(const char* text) {
    if (text == nullptr || *text == '\0') return 0;

    for (const char* c = text; *c != '\0'; ++c) {
        if (*c < '0' || *c > '9') return 0;
    }

    try {
        const long long value = std::stoll(text);
        if (value <= 0 || value > 2147483647LL) return 0;
        return static_cast<int>(value);
    } catch (...) {
        return 0;
    }
}

static void ArgError(const char* value, const char* name) {
    std::cerr << "error: invalid " << name << " value '" << value
              << "'. Expected a positive whole number.\n"
              << "\nRun 'Eleanor datagen --help' for usage.\n";
}

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
    #ifdef TUNING
        SEARCH::RefreshTunableCaches();
    #else
        SEARCH::InitLMRTable();
    #endif

	Board board;

    if (argc > 1) {
        if (std::string(argv[1]) == "bench") {
            RunBenchmark();
        } else if (std::string(argv[1]) == "datagen") {
            if (argc > 2) {
                const std::string arg = argv[2];

                if (arg == "--help" || arg == "-h" || arg == "help") {
                    DATAGEN::PrintUsage();
                    return 0;
                }
            }

            int positions = 1;
            int threads = 1;
            std::string username = "";

            if (argc > 2) {
                positions = ParsePositiveInt(argv[2]);
                if (positions == 0) {
                    ArgError(argv[2], "positions");
                    return 1;
                }
                positions *= 1000;

                if (argc > 3) {
                    threads = ParsePositiveInt(argv[3]);
                    if (threads == 0) {
                        ArgError(argv[3], "threads");
                        return 1;
                    }

                    if (argc > 4) {
                        username = argv[4];
                    }
                }
            }

            // Only the main thread renders, so it never generates anything.
            // Checked outside the argc guards so a bare "datagen" is caught too.
            if (threads < 2) {
                std::cerr << "error: threads must be at least 2 (got " << threads
                          << "). The main thread only renders the progress"
                          << " screen, so " << threads << " leaves no worker to generate.\n"
                          << "\nRun 'Eleanor datagen --help' for usage.\n";
                return 1;
            }

            if (!username.empty()) {
                DATAGEN::RunOnline(username, positions, threads);
            } else {
                DATAGEN::Run(positions, threads);
            }
        }
    } else {
        UCILoop(board);
    }

	return 0;
}
