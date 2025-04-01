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

// Function to hide the cursor on Windows
#ifdef _WIN32
void HideCursor() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;  // Set cursor visibility to false
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

// Function to show the cursor on Windows
void ShowCursor() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = TRUE;  // Set cursor visibility to true
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}
#endif

// Function to hide the cursor on Linux/macOS
void HideCursorLinux() {
    std::cout << "\033[?25l";  // ANSI escape sequence to hide the cursor
}

// Function to show the cursor on Linux/macOS
void ShowCursorLinux() {
    std::cout << "\033[?25h";  // ANSI escape sequence to show the cursor
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

// Serialize Game
std::ostream &operator<<(std::ostream &os, const Game &game) {
    os << game.format;

    uint32_t moveCount = game.moves.size();
    os.write(reinterpret_cast<const char *>(&moveCount), sizeof(moveCount));

    for (const auto &move : game.moves) {
        os << move;
    }

    return os;
}

// Deserialize Game
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

void WriteToFile(const Game &game, const std::string &filename) {
    std::ofstream file(filename, std::ios::binary | std::ios::app);
    if (!file) throw std::runtime_error("Failed to open file for writing");

    // Write MarlinFormat (full format)
    file.write(reinterpret_cast<const char*>(&game.format), sizeof(MarlinFormat));

    for (const auto &move : game.moves) {
        file.write(reinterpret_cast<const char*>(&move), sizeof(ScoredMove));
    }

    uint32_t zero = 0;
    file.write(reinterpret_cast<const char*>(&zero), sizeof(zero));

    file.close();
}

void PrintProgress(int positions, int targetPositions, Stopwatch &stopwatch) {
    double elapsed = stopwatch.GetElapsedSec();
    double positionsPerSec = elapsed > 0 ? positions / elapsed : 0;

    double positionsInThousands = positions / 1000.0;
    double targetInThousands = targetPositions / 1000.0;

    std::cout << "\033[H";
    std::cout << "Datagen: " << std::fixed << std::setprecision(2) << std::setw(6)
              << positionsInThousands << "K positions processed | "
              << std::setw(6) << targetInThousands << "K target" << std::endl;

    std::cout << "Elapsed Time: " << std::fixed << std::setprecision(2) << elapsed << " sec" << std::endl;
    std::cout << "Positions/sec: " << std::fixed << std::setprecision(2) << positionsPerSec << std::endl;
    std::cout.flush();
}

void Run(int targetPositions, int threads) {
    // Clear the console and hide the cursor
    #ifdef _WIN32
        system("cls");
        HideCursor();  // Hide the cursor on Windows
    #else
        std::cout << "\033[2J\033[H";  // Clear console and move cursor to top-left on Linux/macOS
        HideCursorLinux();  // Hide the cursor on Linux/macOS
    #endif

    Stopwatch stopwatch;
    stopwatch.Restart();

    int positions = 0;
    int lastPrintedPosition = 0;

    while (positions < targetPositions * 1000000) {
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

            positions++;
        }

        if (std::abs(safeResults.score) >= std::abs(SEARCH::MATE_SCORE) - SEARCH::MAX_PLY) {
            wdl = board.sideToMove ? 2 : 0; 
        }

        game.format.packFrom(board, staticEval, wdl);
        WriteToFile(game, "data.binpack");

        if (positions / 100 > lastPrintedPosition) {
            lastPrintedPosition = positions / 1000;
            PrintProgress(positions, targetPositions * 1000000, stopwatch);
        }
    }

    // Show the cursor again after the process is done
    #ifdef _WIN32
        ShowCursor();  // Show the cursor on Windows
    #else
        ShowCursorLinux();  // Show the cursor on Linux/macOS
    #endif
}
}