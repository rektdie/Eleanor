#include "utils.h"
#include "movegen.h"

std::vector<std::string> split(std::string_view str, char delim) {
    std::vector<std::string> results;

    std::istringstream stream(std::string{str});

    for (std::string token{}; std::getline(stream, token, delim);) {
        if (token.empty()) continue;

        results.push_back(token);
    }

    return results;
}

int parseSquare(std::string_view str) {
    constexpr std::string_view files = "abcdefgh";

    int file = files.find(str[0]);
    int rank = str[1] - '0' - 1;

    return file + rank * 8;
}

Move parseMove(Board &board, std::string_view str) {
    GenerateMoves(board, board.sideToMove);

    int from = parseSquare(str.substr(0, 2));
    int to = parseSquare(str.substr(2, 2));

    int flag = 0;

    // Promotion
    if (str.length() > 4) {
        if (board.GetPieceColor(from) != board.GetPieceColor(to)) {
            switch(str[4]) {
            case 'r':
                flag = rookPromoCapture;
                break;
            case 'q':
                flag = queenPromoCapture;
                break;
            case 'b':
                flag = bishopPromoCapture;
                break;
            case 'n':
                flag = knightPromoCapture;
                break;
            }
        } else {
            switch(str[4]) {
            case 'r':
                flag = rookPromotion;
                break;
            case 'q':
                flag = queenPromotion;
                break;
            case 'b':
                flag = bishopPromotion;
                break;
            case 'n':
                flag = knightPromotion;
                break;
            }
        }
    } else {
        for (Move move : board.moveList) {
            if (move.MoveFrom() == from && move.MoveTo() == to) {
                flag = move.GetFlags();
                break;
            }
        }
    }

    return Move(from, to, flag);
}
