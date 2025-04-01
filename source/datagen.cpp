#include <random>
#include <cstring>
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
    }
}
}