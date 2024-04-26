#include "evaluate.h"

static int GetMaterialScore(Board &board, int type, int color) {
    int side = color ? -1 : 1;
    
    return (board.pieces[type] & board.colors[color]).PopCount()
        * materialValues[type] * side;
}

static int GetPositionalScore(Board &board, int type, int color) {
    int score = 0;
    int side = color ? -1 : 1;
    
    Bitboard pieces = board.pieces[type] & board.colors[color];

    while (pieces.GetBoard()) {
        int square = pieces.getLS1BIndex();

        // White
        if (!color) {
            score += pieceSquareScores[type][scoreSquares[square]] * side;
        } else {
            score += pieceSquareScores[type][square] * side;
        }

        pieces.PopBit(square);
    }

    return score;
}

int Evaluate(Board &board) {
    int score = 0;

    for (int color = White; color <= Black; color++) {
        for (int type = Pawn; type <= King; type++) {
            score += GetMaterialScore(board, type, color);
            score += GetPositionalScore(board, type, color);
        }
    }

    return score;
}