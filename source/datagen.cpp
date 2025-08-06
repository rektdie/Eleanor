#include <random>
#include <fstream>
#include <iomanip>
#include <thread>
#include <pthread.h>
#include <tuple>
#include <atomic>
#include <mutex>
#include "datagen.h"
#include "movegen.h"
#include "search.h"
#include "movegen.h"
#include "utils.h"

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

static std::mutex fileOpenMutex;

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
        MOVEGEN::GenerateMoves<All>(board);

        std::uniform_int_distribution<int> dist(0, board.currentMoveIndex - 1);
        bool isLegal = board.MakeMove(board.moveList[dist(rng)]);

        if (!isLegal) {
            count--;
            continue;
        }
        ctx->positionHistory[board.positionIndex] = board.hashKey;
        MOVEGEN::GenerateMoves<All>(board);
        if (IsGameOver(board, ctx)) break;
    }
}

static void WriteToFile(std::vector<Game> &gamesBuffer, std::ofstream &file) {
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

    file.write(buffer.data(), buffer.size());
    file.flush();
}

static void PlayGames(int id, std::atomic<int>& positions, std::atomic<bool>& stopFlag) {
    std::vector<Game> gamesBuffer;
    gamesBuffer.reserve(GAME_BUFFER);

    std::string fileName = "thread" + std::to_string(id) + ".binpack";

    std::ofstream file;
    {
        std::lock_guard<std::mutex> lock(fileOpenMutex);
        file.open(fileName, std::ios::app | std::ios::binary);
    }

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << fileName << std::endl;
        exit(-1);
    }

    const int evenityMargin = 200;

    /*
    const int winAdjMoveCount = 3;
    const int winAdjScore = 400;

    const int drawAdjMoveNumber = 40;
    const int drawAdjMoveCount = 8;
    const int drawAdjScore = 10;
    */

    while (!stopFlag) {
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
            SearchResults results = SEARCH::SearchPosition<SEARCH::datagen>(board, SearchParams(), ctx.get());

            if (!results.bestMove) break;
            safeResults = results;

            game.moves.emplace_back(ScoredMove(results.bestMove.ConvertToViriMoveFormat(),
                    UTILS::ConvertToWhiteRelative(board, results.score)));

            board.MakeMove(results.bestMove);
            ctx->positionHistory[board.positionIndex] = board.hashKey;

            positions++;


            MOVEGEN::GenerateMoves<All>(board);
        }

        if (!SEARCH::IsDraw(board, ctx.get())) {
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

    std::vector<pthread_t> workerThreads;

    for (int i = 0; i < threads - 1; ++i) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 2 * 1024 * 1024); // 2 MB stack

        auto* args = new std::tuple<int, std::atomic<int>*, std::atomic<bool>*>(i, &positions, &stopFlag);

        pthread_t thread;
        if (pthread_create(&thread, &attr,
            [](void* a) -> void* {
                auto* tup = static_cast<std::tuple<int, std::atomic<int>*, std::atomic<bool>*>*>(a);
                PlayGames(std::get<0>(*tup), *std::get<1>(*tup), *std::get<2>(*tup));
                delete tup;
                return nullptr;
            }, args) == 0)
        {
            workerThreads.push_back(thread);
        } else {
            std::cerr << "Failed to create thread " << i << "\n";
            delete args;
        }

        pthread_attr_destroy(&attr);
    }

    Stopwatch sw;
    while (positions < targetPositions) {
        PrintProgress(positions, targetPositions, sw, threads);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }


    PrintProgress(positions, targetPositions, sw, threads);

    stopFlag = true;

    for (pthread_t& thread : workerThreads) {
        pthread_join(thread, nullptr);
    }


    #ifdef _WIN32
        ShowCursor();
    #else
        ShowCursorLinux();
    #endif
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

    remainingTime = std::max(0.0, remainingTime);

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
