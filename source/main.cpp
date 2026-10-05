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
#include <vector>

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

static void PrintUsage() {
    std::cout << "Eleanor " << EleanorVersion << " - chess engine" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  Eleanor" << std::endl;
    std::cout << "  Eleanor bench" << std::endl;
    std::cout << "  Eleanor datagen [positions] [threads] [username]" << std::endl;
    std::cout << "  Eleanor datagen [options]" << std::endl;
    std::cout << "  Eleanor --help" << std::endl;
    std::cout << "  Eleanor --version" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  bench                          Run benchmark" << std::endl;
    std::cout << "  datagen                        Generate training data" << std::endl;
    std::cout << "  help                           Show this help" << std::endl;
    std::cout << "  version                        Show version" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help                     Show this help" << std::endl;
    std::cout << "  -v, --version                  Show version" << std::endl;
    std::cout << "  --bench                        Same as bench" << std::endl;
    std::cout << "  --datagen                      Same as datagen" << std::endl;
    std::cout << "  -p, --positions N              Target size in thousands for datagen" << std::endl;
    std::cout << "  -t, --threads N                Threads for datagen or UCI mode" << std::endl;
    std::cout << "  -u, --username NAME            Online mode username for datagen" << std::endl;
    std::cout << "  --hash N                       Hash size in MB for UCI mode" << std::endl;
    std::cout << "  --evalfile PATH                NNUE file to load for UCI mode" << std::endl;
    std::cout << std::endl;
    std::cout << "Datagen examples:" << std::endl;
    std::cout << "  Eleanor datagen 1000 22" << std::endl;
    std::cout << "  Eleanor datagen --positions 1000 --threads 22" << std::endl;
    std::cout << "  Eleanor datagen 1000 22 myuser" << std::endl;
    std::cout << std::endl;
    std::cout << "UCI go:" << std::endl;
    std::cout << "  go depth N" << std::endl;
    std::cout << "  go mate N" << std::endl;
    std::cout << "  go nodes N" << std::endl;
    std::cout << "  go movetime N" << std::endl;
    std::cout << "  go infinite" << std::endl;
    std::cout << "  go searchmoves MOVE ..." << std::endl;
    std::cout << "  go wtime N btime N winc N binc N movestogo N" << std::endl;
}

static void PrintVersion() {
    std::cout << "Eleanor " << EleanorVersion << std::endl;
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
    if (argc <= 1) {
        UCILoop(board);
        return 0;
    }
    std::string first = argv[1];
    if (first == "--help" || first == "-h" || first == "help") {
        PrintUsage();
        return 0;
    }
    if (first == "--version" || first == "-v" || first == "version") {
        PrintVersion();
        return 0;
    }
    if (first == "bench" || first == "--bench") {
        if (argc > 2) {
            std::string second = argv[2];
            if (second == "--help" || second == "-h" || second == "help") {
                PrintUsage();
                return 0;
            }
        }
        RunBenchmark();
        return 0;
    }
    if (first == "datagen" || first == "--datagen") {
        int positions = 0;
        int datagenThreads = 1;
        std::string username = "";
        bool hasPositions = false;
        bool hasThreads = false;
        std::vector<std::string> positional;
        for (int i = 2; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--help" || arg == "-h" || arg == "help") {
                DATAGEN::PrintUsage();
                return 0;
            } else if (arg == "--positions" || arg == "-p") {
                if (i + 1 >= argc) {
                    std::cerr << "error: missing value for '" << arg << "'." << std::endl;
                    std::cerr << std::endl;
                    std::cerr << "Run 'Eleanor datagen --help' for usage." << std::endl;
                    return 1;
                }
                positions = ParsePositiveInt(argv[++i]);
                if (positions == 0) {
                    ArgError(argv[i], "positions");
                    return 1;
                }
                positions *= 1000;
                hasPositions = true;
            } else if (arg.rfind("--positions=", 0) == 0) {
                positions = ParsePositiveInt(arg.substr(12).c_str());
                if (positions == 0) {
                    ArgError(arg.substr(12).c_str(), "positions");
                    return 1;
                }
                positions *= 1000;
                hasPositions = true;
            } else if (arg == "--threads" || arg == "-t") {
                if (i + 1 >= argc) {
                    std::cerr << "error: missing value for '" << arg << "'." << std::endl;
                    std::cerr << std::endl;
                    std::cerr << "Run 'Eleanor datagen --help' for usage." << std::endl;
                    return 1;
                }
                datagenThreads = ParsePositiveInt(argv[++i]);
                if (datagenThreads == 0) {
                    ArgError(argv[i], "threads");
                    return 1;
                }
                hasThreads = true;
            } else if (arg.rfind("--threads=", 0) == 0) {
                datagenThreads = ParsePositiveInt(arg.substr(10).c_str());
                if (datagenThreads == 0) {
                    ArgError(arg.substr(10).c_str(), "threads");
                    return 1;
                }
                hasThreads = true;
            } else if (arg == "--username" || arg == "-u") {
                if (i + 1 >= argc) {
                    std::cerr << "error: missing value for '" << arg << "'." << std::endl;
                    std::cerr << std::endl;
                    std::cerr << "Run 'Eleanor datagen --help' for usage." << std::endl;
                    return 1;
                }
                username = argv[++i];
            } else if (arg.rfind("--username=", 0) == 0) {
                username = arg.substr(11);
            } else if (!arg.empty() && arg[0] == '-') {
                std::cerr << "error: unknown option '" << arg << "'." << std::endl;
                std::cerr << std::endl;
                std::cerr << "Run 'Eleanor datagen --help' for usage." << std::endl;
                return 1;
            } else {
                positional.push_back(arg);
            }
        }
        size_t posIndex = 0;
        if (!hasPositions && posIndex < positional.size()) {
            positions = ParsePositiveInt(positional[posIndex].c_str());
            if (positions == 0) {
                ArgError(positional[posIndex].c_str(), "positions");
                return 1;
            }
            positions *= 1000;
            hasPositions = true;
            posIndex++;
        }
        if (!hasThreads && posIndex < positional.size()) {
            datagenThreads = ParsePositiveInt(positional[posIndex].c_str());
            if (datagenThreads == 0) {
                ArgError(positional[posIndex].c_str(), "threads");
                return 1;
            }
            hasThreads = true;
            posIndex++;
        }
        if (username.empty() && posIndex < positional.size()) {
            username = positional[posIndex];
            posIndex++;
        }
        if (posIndex < positional.size()) {
            std::cerr << "error: unexpected argument '" << positional[posIndex] << "'." << std::endl;
            std::cerr << std::endl;
            std::cerr << "Run 'Eleanor datagen --help' for usage." << std::endl;
            return 1;
        }
        if (!hasPositions)
            positions = 1000;
        if (!hasThreads) {
            std::cerr << "error: threads must be at least 2." << std::endl;
            std::cerr << std::endl;
            std::cerr << "Run 'Eleanor datagen --help' for usage." << std::endl;
            return 1;
        }
        if (datagenThreads < 2) {
            std::cerr << "error: threads must be at least 2 (got " << datagenThreads << "). The main thread only renders the progress screen, so " << datagenThreads << " leaves no worker to generate." << std::endl;
            std::cerr << std::endl;
            std::cerr << "Run 'Eleanor datagen --help' for usage." << std::endl;
            return 1;
        }
        if (!username.empty()) {
            DATAGEN::RunOnline(username, positions, datagenThreads);
        } else {
            DATAGEN::Run(positions, datagenThreads);
        }
        return 0;
    }
    if (!first.empty() && first[0] == '-') {
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--help" || arg == "-h" || arg == "help") {
                PrintUsage();
                return 0;
            } else if (arg == "--version" || arg == "-v" || arg == "version") {
                PrintVersion();
                return 0;
            } else if (arg == "--threads" || arg == "-t") {
                if (i + 1 >= argc) {
                    std::cerr << "error: missing value for '" << arg << "'." << std::endl;
                    std::cerr << std::endl;
                    std::cerr << "Run 'Eleanor --help' for usage." << std::endl;
                    return 1;
                }
                int value = ParsePositiveInt(argv[++i]);
                if (value < 1 || value > 512) {
                    ArgError(argv[i], "threads");
                    return 1;
                }
                threads = value;
            } else if (arg.rfind("--threads=", 0) == 0) {
                int value = ParsePositiveInt(arg.substr(10).c_str());
                if (value < 1 || value > 512) {
                    ArgError(arg.substr(10).c_str(), "threads");
                    return 1;
                }
                threads = value;
            } else if (arg == "--hash") {
                if (i + 1 >= argc) {
                    std::cerr << "error: missing value for '" << arg << "'." << std::endl;
                    std::cerr << std::endl;
                    std::cerr << "Run 'Eleanor --help' for usage." << std::endl;
                    return 1;
                }
                int value = ParsePositiveInt(argv[++i]);
                if (value < 1 || value > 1024) {
                    ArgError(argv[i], "hash");
                    return 1;
                }
                U64 hashSize = (U64(value) * 1000000ULL) / sizeof(TTBucket);
                SharedTT.Resize(hashSize);
            } else if (arg.rfind("--hash=", 0) == 0) {
                int value = ParsePositiveInt(arg.substr(7).c_str());
                if (value < 1 || value > 1024) {
                    ArgError(arg.substr(7).c_str(), "hash");
                    return 1;
                }
                U64 hashSize = (U64(value) * 1000000ULL) / sizeof(TTBucket);
                SharedTT.Resize(hashSize);
            } else if (arg == "--evalfile") {
                if (i + 1 >= argc) {
                    std::cerr << "error: missing value for '" << arg << "'." << std::endl;
                    std::cerr << std::endl;
                    std::cerr << "Run 'Eleanor --help' for usage." << std::endl;
                    return 1;
                }
                NNUE::net.Load(argv[++i]);
            } else if (arg.rfind("--evalfile=", 0) == 0) {
                NNUE::net.Load(arg.substr(11));
            } else {
                std::cerr << "error: unknown option '" << arg << "'." << std::endl;
                std::cerr << std::endl;
                std::cerr << "Run 'Eleanor --help' for usage." << std::endl;
                return 1;
            }
        }
        UCILoop(board);
        return 0;
    }
    std::cerr << "error: unknown command '" << first << "'." << std::endl;
    std::cerr << std::endl;
    std::cerr << "Run 'Eleanor --help' for usage." << std::endl;
    return 1;
}

