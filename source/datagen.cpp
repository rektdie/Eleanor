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
}

static bool IsGameOver(Board &board) {
    // TODO
}

void Run(int games) {
    for (int i = 0; i < games; i++) {
        Board board;
        MarlinFormat position;
        uint8_t wdl = 1;
        positionIndex = 0;
        std::memset(SEARCH::killerMoves, 0, sizeof(SEARCH::killerMoves));
        SEARCH::history.Clear();

        PlayRandMoves(board);

        SEARCH::SearchResults safeResults;
        
        // while (!IsGameOver(board)) {
        //     SEARCH::SearchResults results = SEARCH::SearchPosition<SEARCH::datagen>(board, SearchParams());

        //     if (!results.bestMove) break;
        //     safeResults = results;

        //     board.MakeMove(results.bestMove);
        // }
        
        if (std::abs(safeResults.score) >= SEARCH::inf - SEARCH::MAX_DEPTH) {
            wdl = board.sideToMove ? 2 : 0; 
        }

        position.packFrom(board, HCE::Evaluate(board), wdl);
        board.PrintBoard();
        std::cout << "Eval: " << position.eval << std::endl;
        std::cout << "WDL: " << int(position.wdl) << std::endl;
    }
}
}