#include <random>
#include <fstream>
#include <iomanip>
#include <thread>
#include <tuple>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <csignal>
#include "datagen.h"
#include "movegen.h"
#include "search.h"
#include "movegen.h"
#include "utils.h"

// OS-dependent threading includes
#ifdef _WIN32
    #include <windows.h>
    #include <io.h>  // for _commit
#else
    #include <pthread.h>
    #include <unistd.h>  // for sync
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
#else
static void HideCursor() {
    std::cout << "\033[?25l";
}

static void ShowCursor() {
    std::cout << "\033[?25h";
}
#endif

namespace DATAGEN {

// Signal handling globals
static std::atomic<bool> g_shutdownRequested(false);

// Signal handler for graceful shutdown
static void SignalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n\nShutdown signal received. Saving data safely...\n" << std::flush;
        g_shutdownRequested.store(true);
    }
}

// Setup signal handlers
static void SetupSignalHandlers() {
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    
    #ifndef _WIN32
    std::signal(SIGHUP, SignalHandler);
    #endif
}

static bool IsGameOver(Board &board, SEARCH::SearchContext* ctx) {
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

static void PlayRandMoves(Board &board, SEARCH::SearchContext* ctx) {
    std::random_device dev;
    std::mt19937_64 rng(dev());

    std::uniform_int_distribution<int> moveDist(0, 1);
    bool plusOne = moveDist(rng);

    for (int count = 0; count < RAND_MOVES + plusOne; count++) {
        MOVEGEN::GenerateMoves<All>(board, true);

        std::uniform_int_distribution<int> dist(0, board.currentMoveIndex - 1);
        bool isLegal = board.MakeMove(board.moveList[dist(rng)]);

        if (!isLegal) {
            count--;
            continue;
        }
        ctx->positionHistory[board.positionIndex] = board.hashKey;
        MOVEGEN::GenerateMoves<All>(board, true);
        if (IsGameOver(board, ctx)) break;
    }
}

static void WriteToFile(std::vector<Game> &gamesBuffer, const std::string& basePath, int& fileCounter) {
    if (gamesBuffer.empty()) return;
    
    int32_t zeroes = 0;
    std::vector<char> buffer;

    size_t totalSize = 0;
    for (const Game& game : gamesBuffer) {
        totalSize += sizeof(game.format);
        totalSize += sizeof(ScoredMove) * game.moves.size();
        totalSize += sizeof(zeroes);
    }
    buffer.reserve(totalSize);

    for (const Game& game : gamesBuffer) {
        const char* formatPtr = reinterpret_cast<const char*>(&game.format);
        buffer.insert(buffer.end(), formatPtr, formatPtr + sizeof(game.format));

        const char* movesPtr = reinterpret_cast<const char*>(game.moves.data());
        buffer.insert(buffer.end(), movesPtr, movesPtr + sizeof(ScoredMove) * game.moves.size());

        const char* zeroesPtr = reinterpret_cast<const char*>(&zeroes);
        buffer.insert(buffer.end(), zeroesPtr, zeroesPtr + sizeof(zeroes));
    }

    // Create temporary file name
    std::string tmpFileName = basePath + std::to_string(fileCounter) + ".tmp";
    std::string finalFileName = basePath + std::to_string(fileCounter) + ".binpack";
    
    // Write to temporary file
    std::ofstream tmpFile(tmpFileName, std::ios::binary);
    if (!tmpFile.is_open()) {
        std::cerr << "Failed to create temporary file: " << tmpFileName << std::endl;
        return;
    }
    
    tmpFile.write(buffer.data(), buffer.size());
    tmpFile.flush();
    
    // Ensure data is written to disk (OS-dependent)
    #ifdef _WIN32
    // Windows: flush and close
    tmpFile.close();
    #else
    // Linux: sync before closing
    tmpFile.close();
    sync();
    #endif
    
    // Atomically rename temp file to final file
    std::error_code ec;
    std::filesystem::rename(tmpFileName, finalFileName, ec);
    if (ec) {
        std::cerr << "Failed to rename " << tmpFileName << " to " << finalFileName 
                  << ": " << ec.message() << std::endl;
        // Try to clean up the temp file
        std::filesystem::remove(tmpFileName, ec);
        return;
    }
    
    fileCounter++;
}

inline void MergeThreadFiles() {
    namespace fs = std::filesystem;

    // Create data directory if it doesn't exist
    fs::create_directories("data");

    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&now_time);

    std::ostringstream oss;
    oss << "data/datagen_" 
        << std::put_time(&tm, "%Y-%m-%d_%H-%M") 
        << ".binpack";
    std::string finalFileName = oss.str();

    std::ofstream finalFile(finalFileName, std::ios::binary);
    if (!finalFile.is_open()) {
        std::cerr << "Failed to create final merged file: " << finalFileName << std::endl;
        return;
    }

    fs::path dataDir("data");
    for (const auto& entry : fs::directory_iterator(dataDir)) {
        if (!entry.is_regular_file()) continue;
        std::string filename = entry.path().filename().string();

        // Only process .binpack files (not .tmp files)
        if (filename.find("thread") == 0 && filename.find(".binpack") != std::string::npos) {
            std::ifstream threadFile(entry.path(), std::ios::binary);
            if (!threadFile.is_open()) {
                std::cerr << "Failed to open " << filename << std::endl;
                continue;
            }

            finalFile << threadFile.rdbuf();
            threadFile.close();

            std::error_code ec;
            fs::remove(entry.path(), ec);
            if (ec) {
                std::cerr << "Failed to delete " << filename << ": " << ec.message() << std::endl;
            }
        }
    }

    // Clean up any remaining .tmp files
    for (const auto& entry : fs::directory_iterator(dataDir)) {
        if (!entry.is_regular_file()) continue;
        std::string filename = entry.path().filename().string();
        
        if (filename.find("thread") == 0 && filename.find(".tmp") != std::string::npos) {
            std::error_code ec;
            fs::remove(entry.path(), ec);
            if (ec) {
                std::cerr << "Failed to delete incomplete temp file " << filename << ": " << ec.message() << std::endl;
            }
        }
    }

    finalFile.close();
    std::cout << "Merged all thread files into: " << finalFileName << std::endl;
}

static void PlayGames(int id, std::atomic<int>& positions, std::atomic<bool>& stopFlag) {
    std::vector<Game> gamesBuffer;
    gamesBuffer.reserve(GAME_BUFFER);

    // Create data directory if it doesn't exist
    std::filesystem::create_directories("data");

    std::string basePath = "data/thread" + std::to_string(id) + "_";
    int fileCounter = 0;

    const int evenityMargin = 200;

    try {
        while (!stopFlag && !g_shutdownRequested) {
            Board board;
            auto ctx = std::make_unique<SEARCH::SearchContext>();

            PlayRandMoves(board, ctx.get());
            if (IsGameOver(board, ctx.get())) continue;

            // Filter out games with a highly uneven starting position
            const int startingScore = SEARCH::SearchPosition<SEARCH::datagen>(board, SearchParams(), ctx.get()).score;

            if (std::abs(startingScore) >= evenityMargin) continue;

            ctx->nodes = 0;

            Game game;
            Board startpos = board;

            SearchResults safeResults;

            int staticEval = UTILS::ConvertToWhiteRelative(board, NNUE::net.Evaluate(board));
            int wdl = 1;

            while (!IsGameOver(board, ctx.get())) {
                // Check for shutdown during game generation
                if (g_shutdownRequested) break;
                
                SearchResults results = SEARCH::SearchPosition<SEARCH::datagen>(board, SearchParams(), ctx.get());

                if (!results.bestMove) break;
                safeResults = results;

                game.moves.emplace_back(ScoredMove(results.bestMove.ConvertToViriMoveFormat(),
                        UTILS::ConvertToWhiteRelative(board, results.score)));

                board.MakeMove(results.bestMove);
                ctx->positionHistory[board.positionIndex] = board.hashKey;

                positions++;

                MOVEGEN::GenerateMoves<All>(board, true);
            }

            if (!SEARCH::IsDraw(board, ctx.get())) {
                wdl = board.sideToMove ? 2 : 0;
            }

            game.format.packFrom(startpos, staticEval, wdl);
            gamesBuffer.emplace_back(game);

            if (gamesBuffer.size() >= GAME_BUFFER) {
                WriteToFile(gamesBuffer, basePath, fileCounter);
                gamesBuffer.clear();
            }
        }
    } catch (...) {
        // Ensure we flush on any exception
        if (!gamesBuffer.empty()) {
            WriteToFile(gamesBuffer, basePath, fileCounter);
        }
        throw;
    }

    // Write remaining games to file
    if (gamesBuffer.size() > 0) {
        WriteToFile(gamesBuffer, basePath, fileCounter);
        gamesBuffer.clear();
    }
}

#ifdef _WIN32
// Windows implementation using std::thread
static void CreateWorkerThread(int id, std::atomic<int>& positions, std::atomic<bool>& stopFlag, std::vector<std::thread>& threads) {
    threads.emplace_back(PlayGames, id, std::ref(positions), std::ref(stopFlag));
}
#else
// Unix/Linux implementation using pthread
static void* ThreadFunc(void* arg) {
    auto* tup = static_cast<std::tuple<int, std::atomic<int>*, std::atomic<bool>*>*>(arg);
    PlayGames(std::get<0>(*tup), *std::get<1>(*tup), *std::get<2>(*tup));
    delete tup;
    return nullptr;
}
#endif

void Run(int targetPositions, int threads) {
    // Setup signal handlers first
    SetupSignalHandlers();
    
    #ifdef _WIN32
        system("cls");
        HideCursor();
    #else
        std::cout << "\033[2J\033[H";
        HideCursor();
    #endif

    std::atomic<int> positions = 0;
    std::atomic<bool> stopFlag = false;

#ifdef _WIN32
    // Windows implementation using std::thread
    std::vector<std::thread> workerThreads;
    
    for (int i = 0; i < threads - 1; ++i) {
        CreateWorkerThread(i, positions, stopFlag, workerThreads);
    }
#else
    // Unix/Linux implementation using pthread
    std::vector<pthread_t> workerThreads;

    for (int i = 0; i < threads - 1; ++i) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 8 * 1024 * 1024); // 8 MB stack

        auto* args = new std::tuple<int, std::atomic<int>*, std::atomic<bool>*>(i, &positions, &stopFlag);

        pthread_t thread;
        if (pthread_create(&thread, &attr, ThreadFunc, args) == 0) {
            workerThreads.push_back(thread);
        } else {
            std::cerr << "Failed to create thread " << i << "\n";
            delete args;
        }

        pthread_attr_destroy(&attr);
    }
#endif

    Stopwatch sw;
    while (positions < targetPositions && !g_shutdownRequested) {
        PrintProgress(positions, targetPositions, sw, threads);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    PrintProgress(positions, targetPositions, sw, threads);

    stopFlag = true;

    std::cout << "\n\nWaiting for threads to finish writing..." << std::endl;

#ifdef _WIN32
    // Join Windows threads
    for (std::thread& thread : workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
#else
    // Join pthread threads
    for (pthread_t& thread : workerThreads) {
        pthread_join(thread, nullptr);
    }
#endif

    std::cout << "All threads completed. Merging files..." << std::endl;
    MergeThreadFiles();

    #ifdef _WIN32
        ShowCursor();
    #else
        ShowCursor();
    #endif
    
    std::cout << "\nShutdown complete. All data saved safely." << std::endl;
}

void PrintProgress(int positions, int targetPositions, Stopwatch &stopwatch, int threads) {
    // Define ANSI escape codes for colors as std::string
    const std::string COLOR_RESET = "\033[0m";
    const std::string COLOR_TITLE = "\033[1;36m";    // Bright cyan
    const std::string COLOR_SECTION = "\033[1;33m";  // Bright yellow
    const std::string COLOR_VALUE = "\033[1;32m";    // Bright green
    const std::string COLOR_BAR = "\033[1;34m";      // Bright blue

    static double lastPositions = 0;
    static double lastElapsed = 0;
    static double refreshInterval = 1.0;

    double elapsed = stopwatch.GetElapsedSec();
    double positionsPerSec = 0;

    if (elapsed - lastElapsed >= refreshInterval) {
        positionsPerSec = (positions - lastPositions) / (elapsed - lastElapsed);
        lastPositions = positions;
        lastElapsed = elapsed;
    }

    std::cout << "\033[H";

    int width = 60;

    std::string title = "Eleanor - Datagen";
    int titlePadding = (width - title.length()) / 2;
    std::cout << COLOR_TITLE << std::setw(titlePadding + title.length()) << title << COLOR_RESET << std::endl;

    std::cout << std::string(width, '=') << std::endl;

    std::cout << std::endl;

    std::cout << std::fixed << std::setprecision(0);

    std::ostringstream datagenTextStream;
    datagenTextStream << std::fixed << std::setprecision(2)
                    << "Positions processed: " << positions / 1000.0 << "K | Target: " << targetPositions / 1000.0 << "K";
    std::string datagenText = datagenTextStream.str();
    int datagenPadding = (width - datagenText.length()) / 2;
    std::cout << COLOR_SECTION << std::setw(datagenPadding + datagenText.length()) << datagenText << COLOR_RESET << std::endl;

    std::cout << std::endl;

    std::string elapsedText = "Elapsed Time: " + std::to_string(static_cast<int>(round(elapsed))) + " sec";
    int elapsedPadding = (width - elapsedText.length()) / 2;
    std::cout << COLOR_SECTION << std::setw(elapsedPadding + elapsedText.length()) << elapsedText << COLOR_RESET << std::endl;

    std::string positionsPerSecText = "Positions/sec: " + std::to_string(static_cast<int>(round(positionsPerSec)));
    int positionsPerSecPadding = (width - positionsPerSecText.length()) / 2;
    std::cout << COLOR_VALUE << std::setw(positionsPerSecPadding + positionsPerSecText.length()) << positionsPerSecText << COLOR_RESET << std::endl;

    std::cout << std::endl;

    std::string threadsText = "Threads: " + std::to_string(static_cast<int>(threads));
    int threadsTextPadding = (width - threadsText.length()) / 2;
    std::cout << COLOR_SECTION << std::setw(threadsTextPadding + threadsText.length()) << threadsText << COLOR_RESET << std::endl;

    std::cout << std::endl;

    double remainingTime = 0.0;
    if (positions > 0 && elapsed > 0.0) {
        double positionsPerSec = positions / elapsed;
        remainingTime = (targetPositions - positions) / positionsPerSec;
    }

    remainingTime = (std::max)(0.0, remainingTime);

    int remainingHours = static_cast<int>(remainingTime) / 3600;
    int remainingMinutes = (static_cast<int>(remainingTime) % 3600) / 60;
    int remainingSeconds = static_cast<int>(remainingTime) % 60;

    std::ostringstream remainingTimeStream;
    remainingTimeStream << "Estimated Time Left: "
        << std::setfill('0') << std::setw(2) << remainingHours << "h "
        << std::setw(2) << remainingMinutes << "m "
        << std::setw(2) << remainingSeconds << "s";

    std::string remainingTimeText = remainingTimeStream.str();
    int remainingPadding = (width - remainingTimeText.length()) / 2;
    std::cout << COLOR_SECTION << std::setw(remainingPadding + remainingTimeText.length())
        << remainingTimeText << COLOR_RESET << std::endl;

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
    std::cout << COLOR_BAR << std::setw(progressPadding + progressBar.length()) << progressBar << COLOR_RESET << std::endl;

    std::cout << std::endl;
}

}