#include <random>
#include <cstring>
#include <fstream>
#include "datagen.h"
#include "movegen.h"
#include "evaluate.h"
#include "search.h"
#include "movegen.h"

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


// Serialize MarlinFormat
std::ostream &operator<<(std::ostream &os, const MarlinFormat &format) {
    os.write(reinterpret_cast<const char *>(&format), sizeof(MarlinFormat));
    return os;
}

// Deserialize MarlinFormat
std::istream &operator>>(std::istream &is, MarlinFormat &format) {
    is.read(reinterpret_cast<char *>(&format), sizeof(MarlinFormat));
    return is;
}

// Serialize ScoredMove
std::ostream &operator<<(std::ostream &os, const ScoredMove &move) {
    os.write(reinterpret_cast<const char *>(&move), sizeof(ScoredMove));
    return os;
}

// Deserialize ScoredMove
std::istream &operator>>(std::istream &is, ScoredMove &move) {
    is.read(reinterpret_cast<char *>(&move), sizeof(ScoredMove));
    return is;
}

// Serialize Game
std::ostream &operator<<(std::ostream &os, const Game &game) {
    os << game.format;  // Write MarlinFormat

    uint32_t moveCount = game.moves.size();
    os.write(reinterpret_cast<const char *>(&moveCount), sizeof(moveCount));  // Write move count

    for (const auto &move : game.moves) {
        os << move;  // Write each move
    }

    return os;
}

// Deserialize Game
std::istream &operator>>(std::istream &is, Game &game) {
    is >> game.format;  // Read MarlinFormat

    uint32_t moveCount;
    is.read(reinterpret_cast<char *>(&moveCount), sizeof(moveCount));  // Read move count

    game.moves.resize(moveCount);
    for (auto &move : game.moves) {
        is >> move;  // Read each move
    }

    return is;
}

void WriteToFile(const Game &game, const std::string &filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) throw std::runtime_error("Failed to open file for writing");

    // Write MarlinFormat (full format)
    file.write(reinterpret_cast<const char*>(&game.format), sizeof(MarlinFormat));

    // Write each ScoredMove
    for (const auto &move : game.moves) {
        file.write(reinterpret_cast<const char*>(&move), sizeof(ScoredMove));
    }

    // Write 4 zero bytes at the end
    uint32_t zero = 0;
    file.write(reinterpret_cast<const char*>(&zero), sizeof(zero));

    file.close();
}

void Run(int games) {
    for (int i = 0; i < games; i++) {
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
        }

        if (std::abs(safeResults.score) >= std::abs(SEARCH::MATE_SCORE) - SEARCH::MAX_PLY) {
            wdl = board.sideToMove ? 2 : 0; 
        }

        game.format.packFrom(board, staticEval, wdl);
        WriteToFile(game, "data.binpack");
    }
}
}