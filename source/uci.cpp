#include "uci.h"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <string>
#include <thread>
#include "types.h"
#include "movegen.h"
#include "search.h"
#include "benchmark.h"
#include "perft.h"
#include "utils.h"
#include "datagen.h"

static void ParsePosition(Board &board, std::string_view command, SEARCH::SearchContext* ctx) {
    if (command.find("startpos") != std::string::npos) {
        board.SetByFen(StartingFen);
    }

    int fenIndex = command.find("fen");
    int movesIndex = command.find("moves");

    if (fenIndex != std::string::npos) {
        if (movesIndex != std::string::npos) {
            board.SetByFen(command.substr(fenIndex + 4, movesIndex - 2 - (fenIndex + 3)));
        } else {
            board.SetByFen(command.substr(fenIndex + 4, command.length() - (fenIndex + 3)));
        }
    }

    if (movesIndex != std::string::npos) {
        std::vector<std::string> moves = UTILS::split(command.substr(movesIndex + 6, command.length() - (movesIndex + 5)), ' ');

        for (std::string_view move : moves) {
            board.MakeMove(UTILS::parseMove(board, move));
            ctx->positionHistory[board.positionIndex] = board.hashKey;
        }
    }

    MOVEGEN::GenerateMoves<All>(board);
}

static int ReadParam(std::string param, std::string &command) {
    size_t pos = command.find(param);
    if (pos != std::string::npos) {
        pos += param.length() + 1;
        bool isNegative = false;
        std::string result = "";

        // Check for negative sign
        if (pos < command.length() && command[pos] == '-') {
            isNegative = true;
            pos++;
        }

        // Extract digits
        while (pos < command.length() && std::isdigit(command[pos])) {
            result += command[pos];
            pos++;
        }

        // Ensure result is not empty before calling stoi
        if (!result.empty()) {
            int value = std::stoi(result);
            return isNegative ? -value : value;
        }
    }

    return 0;
}

static void ParseGo(Board &board, std::string &command, SEARCH::SearchContext* ctx) {
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

    if (params.nodes) {
        auto worker = std::thread(SEARCH::SearchPosition<SEARCH::nodesMode>, std::ref(board), params, ctx);
        worker.detach();
    } else {
        auto worker = std::thread(SEARCH::SearchPosition<SEARCH::normal>, std::ref(board), params, ctx);
        worker.detach();
    }
}

static void SetOption(std::string &command, SEARCH::SearchContext* ctx) {
    if (command.find("Hash") != std::string::npos) {
        U64 hashSize = (ReadParam("value", command) * 1000000) / sizeof(TTEntry);
        ctx->TT.Resize(hashSize);
    }
}

static void PrintEngineInfo() {
    std::cout << "id name Eleanor v1.0" << std::endl;
    std::cout << "id author rektdie" << std::endl;
    std::cout << "option name Hash type spin default 8 min 1 max 1024" << std::endl;
    std::cout << "option name Threads type spin default 1 min 1 max 1" << std::endl;
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
            board.SetByFen(StartingFen);
            
            // Clearing
            ctx->TT.Clear();
            
            ctx->killerMoves = {};
            ctx->history.Clear();
            ctx->conthist.Clear();
            ctx->positionHistory = {};
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
            // stop the loop
            break;
        }

        // parse UCI "stop" command
        if (input.find("stop") != std::string::npos) {
            // stop the loop
            ctx->searchStopped = true;
            continue;
        }

        // parse UCI "uci" command
        if (input.find("uci") != std::string::npos) {
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

        if (input.find("datagen") != std::string::npos) {
            DATAGEN::Run(500000*2, 22);
            continue;
        }
    }
}
