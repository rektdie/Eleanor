#include "uci.h"
#include <iostream>
#include <algorithm>
#include <string>
#include <thread>
#include "types.h"
#include "movegen.h"
#include "search.h"
#include "benchmark.h"
#include <cstring>
#include "tt.h"
#include "perft.h"
#include "utils.h"

void ParsePosition(Board &board, const char* command) {
    command += 9; // Skip "position"

    if (strncmp(command, "fen", 3) == 0) {
        command += 4; // Skip "fen "

        std::string fen;
        while (*command && strncmp(command, " moves", 6) != 0) {
            fen += *command;
            command++;
        }

        board.SetByFen(fen.c_str());

        if (strncmp(command, " moves", 6) == 0) {
            command += 6; // Skip " moves"
        }
    } else if (strncmp(command, "startpos", 8) == 0) {
        command += 8; // Skip "startpos"
        board.SetByFen(StartingFen);

        if (strncmp(command, " moves", 6) == 0) {
            command += 6; // Skip " moves"
        }
    }

    while (*command) {
        while (*command == ' ') command++; // Skip spaces

        std::string moveString;
        while (*command && *command != ' ') {
            moveString += *command;
            command++;
        }

        if (!moveString.empty()) {
            board.MakeMove(parseMove(board, moveString));
        }
    }
}

static int ReadParam(std::string param, std::string &command) {
    size_t pos = -1;
    pos = command.find(param);

    if (pos != std::string::npos) {
        std::string result = "";
        pos += param.length() + 1;

        while(pos < command.length() && std::isdigit(command[pos])){
            result += command[pos];
            pos++;
        }
        return stoi(result);
    }

    return 0;
}

void ParseGo(Board &board, std::string &command) {
    SearchParams params;

    params.wtime = ReadParam("wtime", command);
    params.btime = ReadParam("btime", command);
    params.winc = ReadParam("winc", command);
    params.binc = ReadParam("binc", command);
    params.movesToGo = ReadParam("movestogo", command);

    if (command.find("movetime") != std::string::npos) {
        params.wtime = ReadParam("movetime", command);
        params.btime = ReadParam("movetime", command);
    }
    
    auto worker = std::thread(SearchPosition, std::ref(board), params);
    worker.detach();
}

static void SetOption(std::string &command) {
    if (command.find("Hash") != std::string::npos) {
        hashSize = (ReadParam("value", command) * 1000000) / sizeof(TTEntry);
        TT.Resize(hashSize);
    }
}

static void PrintEngineInfo() {
    std::cout << "id name Eleanor" << std::endl;
    std::cout << "id author rektdie" << std::endl;
    std::cout << "option name Hash type spin default 5 min 1 max 64" << std::endl;
    std::cout << "option name Threads type spin default 1 min 1 max 1" << std::endl;
    std::cout << "uciok" << std::endl;
}

void UCILoop(Board &board) {
    // reset STDIN & STDOUT buffers
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    // define user / GUI input buffer
    std::string input = "";

    // main loop
    while (true) {
        // reset user / GUI input
        getline(std::cin, input);

        // make sure output reaches the GUI
        fflush(stdout);

        // make sure input is available
        if (input[0] == '\n') {
            continue;
        }

        // parse UCI "isready" command
        if (input.find("isready") != std::string::npos) {
            std::cout << "readyok" << std::endl;
            continue;
        }

        // parse UCI "position" command
        if (input.find("position") != std::string::npos) {
            ParsePosition(board, input.c_str());
            continue;
        }

        // parse UCI "ucinewgame" command
        if (input.find("ucinewgame") != std::string::npos) {
            board.SetByFen(StartingFen);
            TT.Clear();
            continue;
        }

        // parse UCI "go" command
        if (input.find("go") != std::string::npos) {
            ParseGo(board, input); 
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
            StopSearch();
            continue;
        }

        // parse UCI "uci" command
        if (input.find("uci") != std::string::npos) {
            // print engine info
            PrintEngineInfo();
            continue;
        }

        if (input.find("setoption") != std::string::npos) {
            SetOption(input);
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
    }
}
