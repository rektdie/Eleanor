#include "uci.h"
#include <iostream>
#include <cstring>
#include <string>
#include <tuple>
#include <memory>
#include <algorithm>
#include "types.h"
#include "movegen.h"
#include "search.h"
#include "benchmark.h"
#include "perft.h"
#include "utils.h"
#include "datagen.h"
#include "tunables.h"

// OS-dependent threading includes
#ifdef _WIN32
    #include <thread>
#else
    #include <pthread.h>
#endif

namespace {

constexpr size_t DefaultPositionHistorySize = 1000;

static void EnsurePositionHistory(SEARCH::SearchContext* ctx, int positionIndex) {
    if (positionIndex >= static_cast<int>(ctx->positionHistory.size())) {
        ctx->positionHistory.resize(positionIndex + 100);
    }
}

static void ResetPositionHistory(SEARCH::SearchContext* ctx, const Board& board) {
    const size_t historySize = std::max(DefaultPositionHistorySize, static_cast<size_t>(board.positionIndex + 100));
    ctx->positionHistory.assign(historySize, 0);
    ctx->positionHistory[board.positionIndex] = board.hashKey;
}

#ifdef _WIN32
static std::vector<std::thread> searchThreads;
#else
static std::vector<pthread_t> searchThreads;
#endif

static void JoinSearchThreads() {
#ifdef _WIN32
    for (auto& thread : searchThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
#else
    for (pthread_t& thread : searchThreads) {
        pthread_join(thread, nullptr);
    }
#endif

    searchThreads.clear();
}

static void StopSearchThreads() {
    searchStopped.store(true, std::memory_order_relaxed);
    JoinSearchThreads();
}

} // namespace

static void ParsePosition(Board &board, std::string_view command, SEARCH::SearchContext* ctx) {
    if (command.find("startpos") != std::string::npos) {
        board.SetByFen(StartingFen);
        ResetPositionHistory(ctx, board);
    }

    int fenIndex = command.find("fen");
    int movesIndex = command.find("moves");

    if (fenIndex != std::string::npos) {
        if (movesIndex != std::string::npos) {
            board.SetByFen(command.substr(fenIndex + 4, movesIndex - 2 - (fenIndex + 3)));
        } else {
            board.SetByFen(command.substr(fenIndex + 4, command.length() - (fenIndex + 3)));
        }

        ResetPositionHistory(ctx, board);
    }

    if (movesIndex != std::string::npos) {
        std::vector<std::string> moves = UTILS::split(command.substr(movesIndex + 6, command.length() - (movesIndex + 5)), ' ');

        for (std::string_view move : moves) {
            board.MakeMove(UTILS::parseMove(board, move));
            EnsurePositionHistory(ctx, board.positionIndex);
            ctx->positionHistory[board.positionIndex] = board.hashKey;
        }
    }

    MOVEGEN::GenerateMoves<All>(board, true);
}

static double ReadParam(const std::string& param, const std::string &command) {
    size_t pos = command.find(param);
    if (pos == std::string::npos)
        return 0.0;

    pos += param.length() + 1; // skip "param "
    std::string result;

    // optional sign
    if (pos < command.length() && (command[pos] == '-' || command[pos] == '+')) {
        result += command[pos];
        pos++;
    }

    // digits and optional decimal point
    bool decimalSeen = false;
    while (pos < command.length() && (std::isdigit(command[pos]) || command[pos] == '.')) {
        if (command[pos] == '.') {
            if (decimalSeen) break; // only one decimal point
            decimalSeen = true;
        }
        result += command[pos++];
    }

    if (!result.empty()) {
        try {
            return std::stod(result);
        } catch (const std::out_of_range&) {
            return (result[0] == '-') ? -1e308 : 1e308; // clamp to double range
        } catch (const std::invalid_argument&) {
            return 0.0;
        }
    }

    return 0.0;
}


#ifdef _WIN32
// Windows implementation using std::thread
template <SEARCH::searchMode mode>
static void ThreadFunc(Board board, SearchParams params, SEARCH::SearchContext* ctx) {
    SEARCH::SearchPosition<mode>(board, params, ctx);
    delete ctx;
}

static void StartSearchThread(Board& board, SearchParams params, SEARCH::SearchContext* ctx, int id) {
    SEARCH::SearchContext* ctxCopy = new SEARCH::SearchContext(*ctx);
    if (id == 0)
        ctxCopy->doPrint = true;

    if (params.nodes) {
        searchThreads.emplace_back(ThreadFunc<SEARCH::nodesMode>, board, params, ctxCopy);
    } else {
        searchThreads.emplace_back(ThreadFunc<SEARCH::normal>, board, params, ctxCopy);
    }
}

#else
// Unix/Linux implementation using pthread
template <SEARCH::searchMode mode>
static void* ThreadFunc(void* arg) {
    auto* tup = static_cast<std::tuple<Board, SearchParams, SEARCH::SearchContext*>*>(arg);
    SEARCH::SearchContext* ctx = std::get<2>(*tup);
    SEARCH::SearchPosition<mode>(std::get<0>(*tup), std::get<1>(*tup), ctx);
    delete ctx;
    delete tup;
    return nullptr;
}

static void StartSearchThread(Board& board, SearchParams params, SEARCH::SearchContext* ctx, int id) {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 8 * 1024 * 1024);  // 8 MB stack

    SEARCH::SearchContext* ctxCopy = new SEARCH::SearchContext(*ctx);
    if (id == 0)
        ctxCopy->doPrint = true;

    pthread_t thread;
    int rc = 0;

    if (params.nodes) {
        auto* arg = new std::tuple<Board, SearchParams, SEARCH::SearchContext*>(board, params, ctxCopy);
        rc = pthread_create(&thread, &attr, ThreadFunc<SEARCH::nodesMode>, arg);
        if (rc != 0) {
            delete std::get<2>(*arg);
            delete arg;
            ctxCopy = nullptr;
        }
    } else {
        auto* arg = new std::tuple<Board, SearchParams, SEARCH::SearchContext*>(board, params, ctxCopy);
        rc = pthread_create(&thread, &attr, ThreadFunc<SEARCH::normal>, arg);
        if (rc != 0) {
            delete std::get<2>(*arg);
            delete arg;
            ctxCopy = nullptr;
        }
    }

    pthread_attr_destroy(&attr);

    if (rc == 0) {
        searchThreads.push_back(thread);
    } else {
        delete ctxCopy;
    }
}
#endif

static void ParseGo(Board &board, std::string &command, SEARCH::SearchContext* ctx) {
    StopSearchThreads();
    searchStopped.store(false, std::memory_order_relaxed);
    ctx->TT->IncreaseAge();

    SearchParams params;

    params.wtime = ReadParam("wtime", command);
    params.btime = ReadParam("btime", command);
    params.winc = ReadParam("winc", command);
    params.binc = ReadParam("binc", command);
    params.movesToGo = ReadParam("movestogo", command);
    params.nodes = ReadParam("nodes", command);

    if (command.find("movetime") != std::string::npos) {
        params.wtime = ReadParam("movetime", command);
        params.btime = ReadParam("movetime", command);
    } else if (command.find("infinite") != std::string::npos) {
        params.wtime = 99999999;
        params.btime = 99999999;
    }

    searchThreads.reserve(threads);
    for (int i = 0; i < threads; i++) {
        StartSearchThread(board, params, ctx, i);
    }
}

static void SetOption(std::string& command, SEARCH::SearchContext* ctx) {
    StopSearchThreads();

    if (command.find("Hash") != std::string::npos) {
        U64 hashSize = (ReadParam("value", command) * 1000000ULL) / sizeof(TTEntry);
        ctx->TT->Resize(hashSize);
        return;
    }

    if (command.find("Threads") != std::string::npos) {
        threads = ReadParam("value", command);
        return;
    }

    if (command.find("UCI_ShowWDL") != std::string::npos) {
        UCIShowWDL = command.find("value true") != std::string::npos
            || command.find("value 1") != std::string::npos;
        return;
    }

    #ifdef TUNING

    size_t namePos = command.find("name");
    if (namePos == std::string::npos)
        return;

    namePos += 5; // skip "name "
    size_t valuePos = command.find(" value");
    if (valuePos == std::string::npos)
        return;

    std::string name = command.substr(namePos, valuePos - namePos);
    name.erase(0, name.find_first_not_of(' '));
    name.erase(name.find_last_not_of(' ') + 1);

    double rawValue = ReadParam("value", command);

    #define X_DOUBLE(paramName, default_val, min_val, max_val) \
        if (name == #paramName) { \
            double v = rawValue; \
            if (v < min_val) v = min_val; \
            if (v > max_val) v = max_val; \
            paramName = v; \
            SEARCH::RefreshTunableCaches(); \
            return; \
        }

    #define X_INT(paramName, default_val, min_val, max_val) \
        if (name == #paramName) { \
            int v = static_cast<int>(rawValue); \
            if (v < min_val) v = min_val; \
            if (v > max_val) v = max_val; \
            paramName = v; \
            SEARCH::RefreshTunableCaches(); \
            return; \
        }

    TUNABLE_LIST

    #undef X_DOUBLE
    #undef X_INT
    #endif
}

static void PrintEngineInfo() {
    std::cout << "id name Eleanor v4.1" << std::endl;
    std::cout << "id author rektdie" << std::endl;
    std::cout << "option name Hash type spin default 8 min 1 max 1024" << std::endl;
    std::cout << "option name Threads type spin default 1 min 1 max 512" << std::endl;
    std::cout << "option name UCI_ShowWDL type check default false" << std::endl;

    #ifdef TUNING
        PrintTunablesUCI();
    #endif

    std::cout << "uciok" << std::endl;
}

void UCILoop(Board &board) {
    std::string input = "";

    auto ctx = std::make_unique<SEARCH::SearchContext>();

    // main loop
    while (true) {
        std::getline(std::cin, input);

        // parse UCI "isready" command
        if (input.find("isready") != std::string::npos) {
            std::cout << "readyok" << std::endl;
            continue;
        }

        // parse UCI "position" command
        if (input.find("position") != std::string::npos) {
            ParsePosition(board, input, ctx.get());

            continue;
        }

        // parse UCI "ucinewgame" command
        if (input.find("ucinewgame") != std::string::npos) {
            StopSearchThreads();
            board.SetByFen(StartingFen);

            // Clearing
            ctx->TT->Clear();

            ctx->killerMoves = {};
            ctx->history.Clear();
            ctx->conthist.Clear();
            ctx->corrhist.Clear();
            ResetPositionHistory(ctx.get(), board);
            ctx->ss = {};

            continue;
        }

        // parse UCI "go" command
        if (input.find("go") != std::string::npos) {
            ParseGo(board, input, ctx.get());
            continue;
        }

        // parse UCI "quit" command
        if (input.find("quit") != std::string::npos) {
            StopSearchThreads();
            // stop the loop
            break;
        }

        // parse UCI "stop" command
        if (input.find("stop") != std::string::npos) {
            StopSearchThreads();
            continue;
        }

        // parse UCI "uci" command
        if (input.find("uci") != std::string::npos) {
            UCIEnabled = true;
            // print engine info
            PrintEngineInfo();
            continue;
        }

        if (input.find("setoption") != std::string::npos) {
            SetOption(input, ctx.get());
            continue;
        }

        // Handling non-UCI "bench" command
        if (input.find("bench") != std::string::npos) {
            RunBenchmark();
            continue;
        }

        if (input.find("perft") != std::string::npos) {
            Perft(board, ReadParam("perft", input));
            continue;
        }

        if (input.find("print") != std::string::npos) {
            board.PrintBoard();
            continue;
        }

        if (input.find("listmoves") != std::string::npos) {
            board.ListMoves();
            continue;
        }

        if (input.find("nnue") != std::string::npos) {
            board.PrintNNUE();
            continue;
        }

        if (input.find("tunables") != std::string::npos) {
            PrintTunables();
            continue;
        }

        if (input.find("datagen") != std::string::npos) {
            DATAGEN::Run(500000*2, 22);
            continue;
        }
    }
}
