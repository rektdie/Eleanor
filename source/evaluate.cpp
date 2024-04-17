#include "evaluate.h"

int Evaluate(Board &board) {
    int score = 0;

    for (int color = White; color <= Black; color++) {
        for (int type = Pawn; type <= King; type++) {
            int side = color ? -1 : 1;

            score += (board.pieces[type] & board.colors[color]).PopCount()
                * materialValues[type] * side;
        }
    }

    return score;
}