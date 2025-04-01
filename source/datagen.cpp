#include <random>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <thread>
#include "datagen.h"
#include "movegen.h"
#include "evaluate.h"
#include "search.h"
#include "movegen.h"
#include "stopwatch.h"

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

std::ostream &operator<<(std::ostream &os, const MarlinFormat &format) {
    os.write(reinterpret_cast<const char *>(&format), sizeof(MarlinFormat));
    return os;
}

std::istream &operator>>(std::istream &is, MarlinFormat &format) {
    is.read(reinterpret_cast<char *>(&format), sizeof(MarlinFormat));
    return is;
}

std::ostream &operator<<(std::ostream &os, const ScoredMove &move) {
    os.write(reinterpret_cast<const char *>(&move), sizeof(ScoredMove));
    return os;
}

std::istream &operator>>(std::istream &is, ScoredMove &move) {
    is.read(reinterpret_cast<char *>(&move), sizeof(ScoredMove));
    return is;
}

std::ostream &operator<<(std::ostream &os, const Game &game) {
    os << game.format;

    uint32_t moveCount = game.moves.size();
    os.write(reinterpret_cast<const char *>(&moveCount), sizeof(moveCount));

    for (const auto &move : game.moves) {
        os << move;
    }

    return os;
}

std::istream &operator>>(std::istream &is, Game &game) {
    is >> game.format;

    uint32_t moveCount;
    is.read(reinterpret_cast<char *>(&moveCount), sizeof(moveCount));

    game.moves.resize(moveCount);
    for (auto &move : game.moves) {
        is >> move;
    }

    return is;
}

static void WriteToFile(const Game &game, const std::string &filename) {
    // Create a buffer to store the serialized data [prevents corruption]
    std::vector<char> buffer;

    const size_t formatSize = sizeof(MarlinFormat);
    buffer.resize(formatSize + sizeof(uint32_t) * game.moves.size() + sizeof(uint32_t));

    std::memcpy(buffer.data(), &game.format, formatSize);

    size_t offset = formatSize;

    for (const auto &move : game.moves) {
        std::memcpy(buffer.data() + offset, &move, sizeof(ScoredMove));
        offset += sizeof(ScoredMove);
    }

    uint32_t zero = 0;
    std::memcpy(buffer.data() + offset, &zero, sizeof(zero));

    std::ofstream file(filename, std::ios::binary | std::ios::out | std::ios::app);
    if (!file) throw std::runtime_error("Failed to open file for writing");

    file.write(buffer.data(), buffer.size());

    file.close();
}

void PrintProgress(int positions, int targetPositions, Stopwatch &stopwatch, int threads) {
    double elapsed = stopwatch.GetElapsedSec();
    double positionsPerSec = elapsed > 0 ? positions / elapsed : 0;

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

    std::cout << std::string(width, '-') << std::endl;
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
    stopwatch.Restart();

    int positions = 0;
    int lastPrintedPosition = 0;
    double lastPrintedTime = stopwatch.GetElapsedSec();

    while (positions < targetPositions * 1000) {
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

            game.moves.push_back(ScoredMove(results.bestMove, results.score));
            board.MakeMove(results.bestMove);
            MOVEGEN::GenerateMoves<All>(board);

            // Increment positions, but do not exceed target
            if (positions < targetPositions * 1000) {
                positions++;
            }
        }

        if (std::abs(safeResults.score) >= std::abs(SEARCH::MATE_SCORE) - SEARCH::MAX_PLY) {
            wdl = board.sideToMove ? 2 : 0; 
        }

        game.format.packFrom(board, staticEval, wdl);
        WriteToFile(game, "data.binpack");

        // Print progress every second
        double currentTime = stopwatch.GetElapsedSec();
        if (currentTime - lastPrintedTime >= 1) {
            lastPrintedTime = currentTime;
            PrintProgress(positions, targetPositions * 1000, stopwatch, threads);
        }
    }

    PrintProgress(positions, targetPositions * 1000, stopwatch, threads);

    #ifdef _WIN32
        ShowCursor();
    #else
        ShowCursorLinux();
    #endif
}

}