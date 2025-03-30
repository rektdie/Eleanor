#include <random>
#include "datagen.h"
#include "movegen.h"

namespace DATAGEN {

static void PlayRandMoves(Board &board) {
    std::random_device dev;
    std::mt19937 rng(dev());
    
    for (int count = 0; count < RAND_MOVES; count++) {
        MOVEGEN::GenerateMoves<All>(board);
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, board.currentMoveIndex);

        bool isLegal = board.MakeMove(board.moveList[dist(rng)]);
        if (!isLegal) count--;
    }
}

void Run() {

}
}