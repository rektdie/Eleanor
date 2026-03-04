#include <random>
#include <fstream>
#include <iomanip>
#include <thread>
#include <tuple>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <iostream>
#include <string>
#include <chrono>
#include <sstream>
#include <csignal>
#include <cstdlib>
#include <cerrno>

#include "datagen.h"
#include "movegen.h"
#include "search.h"
#include "utils.h"

// OS-dependent threading includes
#ifdef _WIN32
    #include <windows.h>
    #include <io.h>       // for _commit (if you ever use it)
    #include <process.h>  // _beginthreadex / _endthreadex
#else
    #include <pthread.h>
    #include <unistd.h>  // for sync
#endif

static std::filesystem::path MakeTempPath(const std::string& prefix, const std::string& ext) {
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path();

    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;
    auto name = prefix + std::to_string(dist(gen)) + ext;

    return dir / name;
}

static std::string ShellQuote(const std::string& s) {
#ifdef _WIN32
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else out += c;
    }
    out += "\"";
    return out;
#else
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out += "'\\''";
        else out += c;
    }
    out += "'";
    return out;
#endif
}

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
static void HideCursor() { std::cout << "\033[?25l"; }
static void ShowCursor() { std::cout << "\033[?25h"; }
#endif

namespace DATAGEN {

// ============================================================================
// HTTP CLIENT FUNCTIONS (using system curl commands)
// ============================================================================

static bool PostJSON(const std::string& url, const std::string& jsonBody, std::string& response) {
    namespace fs = std::filesystem;

    fs::path tempFile = MakeTempPath("datagen_request_", ".json");
    fs::path responseFile = MakeTempPath("datagen_response_", ".txt");

    std::ofstream out(tempFile);
    if (!out) {
        std::cerr << "Failed to create temp file for request: " << tempFile.string() << "\n";
        return false;
    }
    out << jsonBody;
    out.close();

    std::ostringstream cmd;
    cmd << "curl -s -X POST "
        << ShellQuote(url) << " "
        << "-H " << ShellQuote("Content-Type: application/json") << " "
        << "-d " << ShellQuote("@" + tempFile.string()) << " "
        << "-o " << ShellQuote(responseFile.string()) << " ";

#ifdef _WIN32
    cmd << "2>nul";
#else
    cmd << "2>/dev/null";
#endif

    int result = system(cmd.str().c_str());

    std::ifstream responseIn(responseFile);
    if (responseIn) {
        std::stringstream buffer;
        buffer << responseIn.rdbuf();
        response = buffer.str();
    } else {
        response.clear();
    }

    std::error_code ec;
    fs::remove(tempFile, ec);
    fs::remove(responseFile, ec);

    return (result == 0);
}

static bool UploadFile(const std::string& url,
                       const std::string& filepath,
                       const std::string& username,
                       int positions) {
    std::ostringstream cmd;

    cmd << "curl -s -X POST "
        << ShellQuote(url) << " "
        << "-F " << ShellQuote("file=@" + filepath) << " "
        << "-F " << ShellQuote("user=" + username) << " "
        << "-F " << ShellQuote("positions=" + std::to_string(positions)) << " ";

#ifdef _WIN32
    cmd << "2>nul";
#else
    cmd << "2>/dev/null";
#endif

    int result = system(cmd.str().c_str());
    return (result == 0);
}

static std::string BuildStartJSON(const std::string& username, int targetPositions) {
    std::ostringstream json;
    json << "{"
         << "\"user\":\"" << username << "\","
         << "\"target\":" << targetPositions
         << "}";
    return json.str();
}

static std::string BuildHeartbeatJSON(const std::string& username, int threads) {
    std::ostringstream json;
    json << "{"
         << "\"user\":\"" << username << "\","
         << "\"threads\":" << threads
         << "}";
    return json.str();
}

static std::string ExtractJSONValue(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";

    pos += searchKey.length();

    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n')) {
        pos++;
    }
    if (pos >= json.length()) return "";

    if (json[pos] == '"') {
        pos++;
        size_t endPos = json.find("\"", pos);
        if (endPos == std::string::npos) return "";
        return json.substr(pos, endPos - pos);
    } else {
        size_t endPos = pos;
        while (endPos < json.length() && json[endPos] != ',' && json[endPos] != '}' && json[endPos] != ']') {
            endPos++;
        }
        return json.substr(pos, endPos - pos);
    }
}

// ============================================================================
// HEARTBEAT THREAD
// ============================================================================

static void HeartbeatThread(const std::string& username, int threads,
                           std::atomic<bool>& stopFlag, const std::string& serverUrl) {
    while (!stopFlag) {
        for (int i = 0; i < 15 && !stopFlag; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        if (stopFlag) break;

        std::string response;
        std::string heartbeatJSON = BuildHeartbeatJSON(username, threads);
        PostJSON(serverUrl + "/api/session/heartbeat", heartbeatJSON, response);
    }
}

// ============================================================================
// EXISTING DATAGEN FUNCTIONS
// ============================================================================

// Signal handling globals
static std::atomic<bool> g_shutdownRequested(false);

static void SignalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n\nShutdown signal received. Saving data safely...\n" << std::flush;
        g_shutdownRequested.store(true);
    }
}

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
        if (!board.IsLegal(board.moveList[i])) continue;
        Board copy = board;
        copy.MakeMove(board.moveList[i]);
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

        if (board.currentMoveIndex <= 0) {
            break;
        }

        std::uniform_int_distribution<int> dist(0, board.currentMoveIndex - 1);

        Move currMove = board.moveList[dist(rng)];
        if (!board.IsLegal(currMove))  {
            count--;
            continue;
        }

        board.MakeMove(currMove);

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

    std::string tmpFileName = basePath + std::to_string(fileCounter) + ".tmp";
    std::string finalFileName = basePath + std::to_string(fileCounter) + ".binpack";

    std::ofstream tmpFile(tmpFileName, std::ios::binary);
    if (!tmpFile.is_open()) {
        std::cerr << "Failed to create temporary file: " << tmpFileName << std::endl;
        return;
    }

    tmpFile.write(buffer.data(), buffer.size());
    tmpFile.flush();

#ifdef _WIN32
    tmpFile.close();
#else
    tmpFile.close();
    sync();
#endif

    std::error_code ec;
    std::filesystem::rename(tmpFileName, finalFileName, ec);
    if (ec) {
        std::cerr << "Failed to rename " << tmpFileName << " to " << finalFileName
                  << ": " << ec.message() << std::endl;
        std::filesystem::remove(tmpFileName, ec);
        return;
    }

    fileCounter++;
}

static std::string MergeThreadFiles() {
    namespace fs = std::filesystem;

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
        return "";
    }

    fs::path dataDir("data");
    for (const auto& entry : fs::directory_iterator(dataDir)) {
        if (!entry.is_regular_file()) continue;
        std::string filename = entry.path().filename().string();

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
    return finalFileName;
}

static void PlayGames(int id, std::atomic<int>& positions, std::atomic<bool>& stopFlag) {
    std::vector<Game> gamesBuffer;
    gamesBuffer.reserve(GAME_BUFFER);

    std::filesystem::create_directories("data");

    std::string basePath = "data/thread" + std::to_string(id) + "_";
    int fileCounter = 0;

    const int evenityMargin = 200;

    try {
        while (!stopFlag && !g_shutdownRequested) {
            Board board;
            TTable DatagenTT;
            auto ctx = std::make_unique<SEARCH::SearchContext>();
            ctx->TT = &DatagenTT;

            PlayRandMoves(board, ctx.get());
            if (IsGameOver(board, ctx.get())) continue;

            const int startingScore = SEARCH::SearchPosition<SEARCH::datagen>(board, SearchParams(), ctx.get()).score;

            if (std::abs(startingScore) >= evenityMargin) continue;

            ctx->nodes = 0;

            Game game;
            Board startpos = board;

            SearchResults safeResults;

            int staticEval = UTILS::ConvertToWhiteRelative(board, NNUE::net.Evaluate(board));
            int wdl = 1;

            while (!IsGameOver(board, ctx.get())) {
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
        if (!gamesBuffer.empty()) {
            WriteToFile(gamesBuffer, basePath, fileCounter);
        }
        throw;
    }

    if (!gamesBuffer.empty()) {
        WriteToFile(gamesBuffer, basePath, fileCounter);
        gamesBuffer.clear();
    }

    if (!stopFlag.load() && !g_shutdownRequested.load()) {
        std::abort();
    }
}

#ifdef _WIN32

struct WinThreadArgs {
    int id;
    std::atomic<int>* positions;
    std::atomic<bool>* stopFlag;
};

static unsigned __stdcall WinThreadEntry(void* p) {
    std::unique_ptr<WinThreadArgs> args(static_cast<WinThreadArgs*>(p));
    PlayGames(args->id, *args->positions, *args->stopFlag);
    _endthreadex(0);
    return 0;
}

static bool CreateWorkerThread(int id,
                               std::atomic<int>& positions,
                               std::atomic<bool>& stopFlag,
                               std::vector<HANDLE>& threads,
                               size_t stackSizeBytes) {
    auto* args = new WinThreadArgs{ id, &positions, &stopFlag };

    unsigned tid = 0;
    HANDLE h = reinterpret_cast<HANDLE>(
        _beginthreadex(
            nullptr,
            static_cast<unsigned>(stackSizeBytes),
            &WinThreadEntry,
            args,
            0,
            &tid
        )
    );

    if (!h) {
        std::cerr << "Failed to create thread " << id << " (errno=" << errno << ")\n";
        delete args;
        return false;
    }

    threads.push_back(h);
    return true;
}

static void JoinWorkerThreads(std::vector<HANDLE>& threads) {
    for (HANDLE h : threads) {
        if (h) {
            WaitForSingleObject(h, INFINITE);
            CloseHandle(h);
        }
    }
    threads.clear();
}

#else

static void* ThreadFunc(void* arg) {
    auto* tup = static_cast<std::tuple<int, std::atomic<int>*, std::atomic<bool>*>*>(arg);
    PlayGames(std::get<0>(*tup), *std::get<1>(*tup), *std::get<2>(*tup));
    delete tup;
    return nullptr;
}

#endif

// ============================================================================
// MAIN RUN FUNCTIONS
// ============================================================================

void Run(int targetPositions, int threads) {
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
    std::vector<HANDLE> workerThreads;
    workerThreads.reserve((threads > 1) ? (threads - 1) : 0);

    const size_t stackSizeBytes = 8ull * 1024ull * 1024ull; // change to 16/32 MiB if needed

    for (int i = 0; i < threads - 1; ++i) {
        CreateWorkerThread(i, positions, stopFlag, workerThreads, stackSizeBytes);
    }
#else
    std::vector<pthread_t> workerThreads;

    for (int i = 0; i < threads - 1; ++i) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 8 * 1024 * 1024);

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
    JoinWorkerThreads(workerThreads);
#else
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

void RunOnline(const std::string& username, int targetPositions, int threads) {
    const std::string SERVER_URL = "http://13.60.26.163:3000";

    SetupSignalHandlers();

    std::cout << "=== ONLINE MODE ===" << std::endl;
    std::cout << "User: " << username << std::endl;
    std::cout << "Target positions per cycle: " << targetPositions << std::endl;
    std::cout << "Threads: " << threads << std::endl;
    std::cout << "===================" << std::endl << std::endl;

    while (!g_shutdownRequested) {
        std::cout << "Sending heartbeat..." << std::endl;
        std::string heartbeatResponse;
        std::string heartbeatJSON = BuildHeartbeatJSON(username, threads);

        if (!PostJSON(SERVER_URL + "/api/session/heartbeat", heartbeatJSON, heartbeatResponse)) {
            std::cerr << "Failed to send heartbeat. Retrying in 10 seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(10));
            continue;
        }

        std::cout << "Heartbeat response: " << heartbeatResponse << std::endl;

        std::string startResponse;
        std::string startJSON = BuildStartJSON(username, targetPositions);

        std::cout << "Starting new session..." << std::endl;
        if (!PostJSON(SERVER_URL + "/api/session/start", startJSON, startResponse)) {
            std::cerr << "Failed to start session. Retrying in 10 seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(10));
            continue;
        }

        std::cout << "Server response: " << startResponse << std::endl;

        std::string sessionId = ExtractJSONValue(startResponse, "session_id");
        if (sessionId.empty()) {
            std::cerr << "Failed to parse session ID. Retrying..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(10));
            continue;
        }

        std::cout << "Session ID: " << sessionId << std::endl;

        std::atomic<bool> heartbeatStop(false);
        std::thread heartbeat(HeartbeatThread, username, threads, std::ref(heartbeatStop), SERVER_URL);

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
        std::vector<HANDLE> workerThreads;
        workerThreads.reserve((threads > 1) ? (threads - 1) : 0);

        const size_t stackSizeBytes = 8ull * 1024ull * 1024ull; // change to 16/32 MiB if needed

        for (int i = 0; i < threads - 1; ++i) {
            CreateWorkerThread(i, positions, stopFlag, workerThreads, stackSizeBytes);
        }
#else
        std::vector<pthread_t> workerThreads;

        for (int i = 0; i < threads - 1; ++i) {
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setstacksize(&attr, 8 * 1024 * 1024);

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
        JoinWorkerThreads(workerThreads);
#else
        for (pthread_t& thread : workerThreads) {
            pthread_join(thread, nullptr);
        }
#endif

        heartbeatStop = true;
        if (heartbeat.joinable()) {
            heartbeat.join();
        }

        if (g_shutdownRequested) break;

        std::cout << "All threads completed. Merging files..." << std::endl;
        std::string mergedFile = MergeThreadFiles();

#ifdef _WIN32
        ShowCursor();
#else
        ShowCursor();
#endif

        if (!mergedFile.empty()) {
            std::cout << "Uploading file to server..." << std::endl;
            std::string uploadUrl = SERVER_URL + "/api/upload/" + sessionId;

            if (UploadFile(uploadUrl, mergedFile, username, positions.load())) {
                std::cout << "Upload successful!" << std::endl;

                std::error_code ec;
                std::filesystem::remove(mergedFile, ec);
                if (!ec) {
                    std::cout << "Local file cleaned up." << std::endl;
                }
            } else {
                std::cerr << "Upload failed. File kept locally: " << mergedFile << std::endl;
            }
        }

        if (g_shutdownRequested) break;

        std::cout << "\n=== Starting next cycle ===" << std::endl << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    std::cout << "\nOnline mode shutdown complete." << std::endl;
}

void PrintProgress(int positions, int targetPositions, Stopwatch &stopwatch, int threads) {
    const std::string COLOR_RESET = "\033[0m";
    const std::string COLOR_TITLE = "\033[1;36m";
    const std::string COLOR_SECTION = "\033[1;33m";
    const std::string COLOR_VALUE = "\033[1;32m";
    const std::string COLOR_BAR = "\033[1;34m";

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
    int titlePadding = (width - static_cast<int>(title.length())) / 2;
    std::cout << COLOR_TITLE << std::setw(titlePadding + static_cast<int>(title.length())) << title << COLOR_RESET << std::endl;

    std::cout << std::string(width, '=') << std::endl;
    std::cout << std::endl;

    std::cout << std::fixed << std::setprecision(0);

    std::ostringstream datagenTextStream;
    datagenTextStream << std::fixed << std::setprecision(2)
                      << "Positions processed: " << positions / 1000.0 << "K | Target: " << targetPositions / 1000.0 << "K";
    std::string datagenText = datagenTextStream.str();
    int datagenPadding = (width - static_cast<int>(datagenText.length())) / 2;
    std::cout << COLOR_SECTION << std::setw(datagenPadding + static_cast<int>(datagenText.length())) << datagenText << COLOR_RESET << std::endl;

    std::cout << std::endl;

    std::string elapsedText = "Elapsed Time: " + std::to_string(static_cast<int>(round(elapsed))) + " sec";
    int elapsedPadding = (width - static_cast<int>(elapsedText.length())) / 2;
    std::cout << COLOR_SECTION << std::setw(elapsedPadding + static_cast<int>(elapsedText.length())) << elapsedText << COLOR_RESET << std::endl;

    std::string positionsPerSecText = "Positions/sec: " + std::to_string(static_cast<int>(round(positionsPerSec)));
    int positionsPerSecPadding = (width - static_cast<int>(positionsPerSecText.length())) / 2;
    std::cout << COLOR_VALUE << std::setw(positionsPerSecPadding + static_cast<int>(positionsPerSecText.length())) << positionsPerSecText << COLOR_RESET << std::endl;

    std::cout << std::endl;

    std::string threadsText = "Threads: " + std::to_string(static_cast<int>(threads));
    int threadsTextPadding = (width - static_cast<int>(threadsText.length())) / 2;
    std::cout << COLOR_SECTION << std::setw(threadsTextPadding + static_cast<int>(threadsText.length())) << threadsText << COLOR_RESET << std::endl;

    std::cout << std::endl;

    double remainingTime = 0.0;
    if (positions > 0 && elapsed > 0.0) {
        double pps = positions / elapsed;
        remainingTime = (targetPositions - positions) / pps;
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
    int remainingPadding = (width - static_cast<int>(remainingTimeText.length())) / 2;
    std::cout << COLOR_SECTION
              << std::setw(remainingPadding + static_cast<int>(remainingTimeText.length()))
              << remainingTimeText << COLOR_RESET << std::endl;

    std::cout << std::endl;

    int barWidth = 50;
    double progress = (double)positions / targetPositions;
    int pos = (int)(progress * barWidth);

    std::string progressBar = "[";

    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) progressBar += "=";
        else if (i == pos) progressBar += ">";
        else progressBar += " ";
    }

    progressBar += "] " + std::to_string(static_cast<int>(round(progress * 100.0))) + " %";

    int progressPadding = (width - static_cast<int>(progressBar.length())) / 2;
    std::cout << COLOR_BAR << std::setw(progressPadding + static_cast<int>(progressBar.length()))
              << progressBar << COLOR_RESET << std::endl;

    std::cout << std::endl;
}

} // namespace DATAGEN