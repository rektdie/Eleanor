#include "evaluate.h"

namespace HCE {

static int GetPhaseWeight(Board &board, int color) {
    int phase = 0;
    
    // For all piece types other than pawn and king
    for (int piece = Knight; piece <= Queen; piece++) {
        phase += board.pieces[piece].PopCount() * phaseValues[piece - 1];
    }

    return phase;
}

static int GetMaterialScore(Board &board, int type, int color) {
    int side = color ? -1 : 1;
    
    return (board.pieces[type] & board.colors[color]).PopCount()
        * materialValues[type] * side;
}

static int GetPositionalScore(Board &board, int type, int color) {
    int middleEval = 0;
    int endEval = 0;

    int phaseWeight = GetPhaseWeight(board, color);

    int side = color ? -1 : 1;
    
    Bitboard pieces = board.pieces[type] & board.colors[color];

    while (pieces) {
        int square = pieces.getLS1BIndex();

        // White
        if (!color) {
            middleEval += middlePieceScores[type][scoreSquares[square]] * side;
            endEval += endPieceSquares[type][scoreSquares[square]] * side;
        } else {
            middleEval += middlePieceScores[type][square] * side;
            endEval += endPieceSquares[type][square] * side;
        }

        pieces.PopBit(square);
    }

    int finalEval = (middleEval * phaseWeight + endEval * (phaseMax - phaseWeight)) / phaseMax;

    return finalEval;
}

int Evaluate(Board &board) {
    int score = 0;

    for (int color = White; color <= Black; color++) {
        for (int type = Pawn; type <= King; type++) {
            score += GetMaterialScore(board, type, color);
            score += GetPositionalScore(board, type, color);
        }
    }

    return board.sideToMove ? -score : score;
}
}