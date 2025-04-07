#include <random>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <thread>
#include <atomic>
#include <mutex>
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

// Atomic bool for locking
std::atomic<bool> bufferLockFlag(false);

static void PlayRandMoves(Board &board) {
    std::random_device dev;
    std::mt19937 rng(dev());
    
    for (int count = 0; count < RAND_MOVES; count++) {
        MOVEGEN::GenerateMoves<All>(board);
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, board.currentMoveIndex - 1);

        bool isLegal = board.MakeMove(board.moveList[dist(rng)]);

        if (!isLegal) count--;
    }

    MOVEGEN::GenerateMoves<All>(board);
}

static bool IsGameOver(Board &board) {
    bool isDraw = SEARCH::IsDraw(board);
    if (isDraw) return true;

    int moveSeen = 0;

    for (int i = 0; i < board.currentMoveIndex; i++) {
        Board copy = board;
        bool isLegal = copy.MakeMove(board.moveList[i]);

        if (!isLegal) continue;
        positionIndex--;
        moveSeen++;
    }

    if (moveSeen == 0) return true;
    return false;
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

void PlayGame(std::atomic<int>& positions, U64 targetPositions, std::vector<Game> &gamesBuffer, 
              std::ofstream &file, std::atomic<bool>& shouldStop) {
    while (!shouldStop.load(std::memory_order_relaxed) && 
           positions.load(std::memory_order_relaxed) < targetPositions) {
        Game game;
        Board board;

        uint8_t wdl = 1;
        positionIndex = 0;
        std::memset(SEARCH::killerMoves, 0, sizeof(SEARCH::killerMoves));
        SEARCH::history.Clear();

        PlayRandMoves(board);

        int staticEval = HCE::Evaluate(board);

        SEARCH::SearchResults safeResults;

        MOVEGEN::GenerateMoves<All>(board);

        while (!IsGameOver(board)) {
            SEARCH::SearchResults results = SEARCH::SearchPosition<SEARCH::datagen>(board, SearchParams());

            if (!results.bestMove) break;
            safeResults = results;

            game.moves.push_back(ScoredMove(results.bestMove.ConvertToViriMoveFormat(), UTILS::ConvertToWhiteRelative(board, results.score)));
            board.MakeMove(results.bestMove);
            MOVEGEN::GenerateMoves<All>(board);

            positions.fetch_add(1, std::memory_order_relaxed);

            if (std::abs(safeResults.score) >= std::abs(SEARCH::MATE_SCORE) - SEARCH::MAX_PLY) {
                wdl = board.sideToMove ? 2 : 0; 
            }
        }
        
        // Only add the game to buffer if we haven't been told to stop
        if (!shouldStop.load(std::memory_order_relaxed)) {
            game.format.packFrom(board, UTILS::ConvertToWhiteRelative(board, staticEval), wdl);

            // Wait until the lock is available
            while (!shouldStop.load(std::memory_order_relaxed) && 
                   bufferLockFlag.exchange(true, std::memory_order_acquire)) {
                // Spin-wait
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            // One final check before writing
            if (!shouldStop.load(std::memory_order_relaxed)) {
                gamesBuffer.push_back(game);

                if (gamesBuffer.size() >= GAME_BUFFER) {
                    WriteToFile(gamesBuffer, file);
                    gamesBuffer.clear();
                }
            }

            // Unlock the buffer after writing
            bufferLockFlag.store(false, std::memory_order_release);
        }
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

    Stopwatch stopwatch;

    std::atomic<int> positions = 0;
    std::atomic<bool> shouldStop(false);
    std::vector<Game> gamesBuffer;
    std::vector<std::thread> workers;

    std::ofstream file("data.binpack", std::ios::app | std::ios::binary);

    // Create and start worker threads
    for (int i = 0; i < threads; i++) {
        workers.push_back(std::thread(PlayGame, std::ref(positions), 
                         targetPositions * 1000, std::ref(gamesBuffer), 
                         std::ref(file), std::ref(shouldStop)));
    }

    int lastPrintedPosition = 0;
    double lastPrintedTime = stopwatch.GetElapsedSec();

    // Monitor progress
    while (positions.load(std::memory_order_relaxed) < targetPositions * 1000) {
        PrintProgress(positions, targetPositions * 1000, stopwatch, threads);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // Signal threads to stop
    shouldStop.store(true, std::memory_order_relaxed);
    
    // Wait for all threads to finish
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    // Final progress update and buffer flush
    PrintProgress(positions, targetPositions * 1000, stopwatch, threads);
    
    // Final check - lock buffer to ensure exclusive access
    while (bufferLockFlag.exchange(true, std::memory_order_acquire)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    WriteToFile(gamesBuffer, file);
    bufferLockFlag.store(false, std::memory_order_release);

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