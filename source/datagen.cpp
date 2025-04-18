#include <random>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <thread>
#include <atomic>
#include "datagen.h"
#include "movegen.h"
#include "evaluate.h"
#include "search.h"
#include "movegen.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
static void HideCursor() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

static void ShowCursor() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}
#endif

static void HideCursorLinux() {
    std::cout << "\033[?25l";
}

static void ShowCursorLinux() {
    std::cout << "\033[?25h";
}

namespace DATAGEN {

static bool IsGameOver(Board &board, SEARCH::SearchContext& ctx) {
    if (SEARCH::IsDraw(board, ctx)) return true;

    int moveSeen = 0;

    for (int i = 0; i < board.currentMoveIndex; i++) {
        Board copy = board;
        bool isLegal = copy.MakeMove(board.moveList[i]);

        if (!isLegal) continue;
        moveSeen++; 
    }

    return moveSeen == 0;
}

static void PlayRandMoves(Board &board, SEARCH::SearchContext& ctx) {
    std::random_device dev;
    std::mt19937_64 rng(dev());
    
    for (int count = 0; count < RAND_MOVES; count++) {
        MOVEGEN::GenerateMoves<All>(board);
        std::uniform_int_distribution<int> dist(0, board.currentMoveIndex - 1);

        bool isLegal = board.MakeMove(board.moveList[dist(rng)]);

        if (!isLegal) {
            count--;
            continue;
        }
        ctx.positionHistory[board.positionIndex] = board.hashKey;
        MOVEGEN::GenerateMoves<All>(board);
        if (IsGameOver(board, ctx)) break;
    }
}

static void WriteToFile(std::vector<Game> &gamesBuffer, std::ofstream &file) {
    int32_t zeroes = 0;
    for (const Game& game : gamesBuffer) {
        file.write(reinterpret_cast<const char*>(&game.format), sizeof(game.format));
        file.write(reinterpret_cast<const char*>(game.moves.data()), sizeof(ScoredMove) * game.moves.size());
        file.write(reinterpret_cast<const char*>(&zeroes), 4);
    }
    file.flush();
}

static void PlayGames(int id, std::atomic<int>& positions, std::atomic<bool>& stopFlag) {
    std::vector<Game> gamesBuffer;
    gamesBuffer.reserve(GAME_BUFFER);

    std::string fileName = "thread" + std::to_string(id) + ".binpack";

    
    std::ofstream file(fileName, std::ios::app | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << fileName << std::endl;
        exit(-1);
    }

    while (!stopFlag) {
        Game game;
        Board board;
        SEARCH::SearchContext ctx;
        PlayRandMoves(board, ctx);
        if (IsGameOver(board, ctx)) continue;

        Board startpos = board;

        SearchResults safeResults;

        int staticEval = UTILS::ConvertToWhiteRelative(board, HCE::Evaluate(board));
        int wdl = 1;

        while (!IsGameOver(board, ctx)) {
            SearchResults results = SEARCH::SearchPosition<SEARCH::datagen>(board, SearchParams(), ctx);

            if (!results.bestMove) break;
            safeResults = results;
            board.MakeMove(results.bestMove);
            ctx.positionHistory[board.positionIndex] = board.hashKey;

            game.moves.emplace_back(ScoredMove(results.bestMove.ConvertToViriMoveFormat(),
                    UTILS::ConvertToWhiteRelative(board, results.score)));

            positions++;

            MOVEGEN::GenerateMoves<All>(board);
        }

        if (std::abs(safeResults.score) >= std::abs(SEARCH::MATE_SCORE) - SEARCH::MAX_PLY) {
            wdl = board.sideToMove ? 2 : 0; 
        }

        game.format.packFrom(startpos, staticEval, wdl);
        gamesBuffer.emplace_back(game);

        if (gamesBuffer.size() >= GAME_BUFFER) {
            WriteToFile(gamesBuffer, file);
            gamesBuffer.clear();
        }
    }

    // Write remaining games to file
    if (gamesBuffer.size() > 0) {
        WriteToFile(gamesBuffer, file);
        gamesBuffer.clear();
    }
}

void Run(int targetPositions, int threads) {
    #ifdef _WIN32
        system("cls");
        HideCursor();
    #else
        std::cout << "\033[2J\033[H";
        HideCursorLinux();
    #endif

    std::atomic<int> positions = 0;
    std::atomic<bool> stopFlag = false;

    std::vector<std::thread> workerThreads;

    // Start N-1 threads
    for (int i = 0; i < threads-1; i++) {
        workerThreads.emplace_back(std::thread(PlayGames, i, std::ref(positions), std::ref(stopFlag)));
    }

    Stopwatch sw;
    while (positions < targetPositions) {
        PrintProgress(positions, targetPositions, sw, threads);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }


    PrintProgress(positions, targetPositions, sw, threads);

    stopFlag = true;

    for (auto& thread : workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }


    #ifdef _WIN32 
        ShowCursor();
    #else
        ShowCursorLinux();
    #endif
}

void PrintProgress(int positions, int targetPositions, Stopwatch &stopwatch, int threads) {
    static double lastPositions = 0; // To track how many positions were processed last time
    static double lastElapsed = 0; // To track the last elapsed time for more frequent updates
    static double refreshInterval = 1.0; // Update every second

    double elapsed = stopwatch.GetElapsedSec();
    double positionsPerSec = 0;

    // Update positionsPerSec if the time difference is above the threshold
    if (elapsed - lastElapsed >= refreshInterval) {
        positionsPerSec = (positions - lastPositions) / (elapsed - lastElapsed);
        lastPositions = positions;
        lastElapsed = elapsed;
    }

    int positionsInThousands = static_cast<int>(round(positions / 1000.0));
    int targetInThousands = static_cast<int>(round(targetPositions / 1000.0));

    std::cout << "\033[H";

    int width = 60;

    std::string title = "Eleanor - Datagen";
    int titlePadding = (width - title.length()) / 2;
    std::cout << std::setw(titlePadding + title.length()) << title << std::endl;

    std::cout << std::string(width, '=') << std::endl;

    std::cout << std::endl;

    std::cout << std::fixed << std::setprecision(0);

    std::ostringstream datagenTextStream;
    datagenTextStream << std::fixed << std::setprecision(2) 
                    << "Positions processed: " << positions / 1000.0 << "K | Target: " << targetPositions / 1000.0 << "K";
    std::string datagenText = datagenTextStream.str();
    int datagenPadding = (width - datagenText.length()) / 2;
    std::cout << std::setw(datagenPadding + datagenText.length()) << datagenText << std::endl;

    std::cout << std::endl;

    std::string elapsedText = "Elapsed Time: " + std::to_string(static_cast<int>(round(elapsed))) + " sec";
    int elapsedPadding = (width - elapsedText.length()) / 2;
    std::cout << std::setw(elapsedPadding + elapsedText.length()) << elapsedText << std::endl;

    std::string positionsPerSecText = "Positions/sec: " + std::to_string(static_cast<int>(round(positionsPerSec)));
    int positionsPerSecPadding = (width - positionsPerSecText.length()) / 2;
    std::cout << std::setw(positionsPerSecPadding + positionsPerSecText.length()) << positionsPerSecText << std::endl;

    std::cout << std::endl;

    std::string threadsText = "Threads: " + std::to_string(static_cast<int>(threads));
    int threadsTextPadding = (width - threadsText.length()) / 2;
    std::cout << std::setw(threadsTextPadding + threadsText.length()) << threadsText << std::endl;

    std::cout << std::endl;

    int barWidth = 50;
    double progress = (double)positions / targetPositions;
    int pos = (int)(progress * barWidth);

    std::string progressBar = "[";

    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) {
            progressBar += "=";
        } else if (i == pos) {
            progressBar += ">";
        } else {
            progressBar += " ";
        }
    }

    progressBar += "] " + std::to_string(static_cast<int>(round(progress * 100.0))) + " %";

    int progressPadding = (width - progressBar.length()) / 2;
    std::cout << std::setw(progressPadding + progressBar.length()) << progressBar << std::endl;

    std::cout << std::endl;
}

}
