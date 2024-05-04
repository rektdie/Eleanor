#include "uci.h"
#include <iostream>
#include <algorithm>
#include <string>
#include "types.h"
#include "movegen.h"

Move ParseMove(Board &board, const char* moveString) {
    GenerateMoves(board, board.sideToMove);

    // Source square
    std::string fromString = "";
    fromString += moveString[0];
    fromString += moveString[1];

    auto it = std::find(squareCoords.begin(), squareCoords.end(), fromString);

    int from = std::distance(squareCoords.begin(), it);

    // Target square
    std::string toString = ""; 
    toString += moveString[2];
    toString += moveString[3];

    it = std::find(squareCoords.begin(), squareCoords.end(), toString);

    int to = std::distance(squareCoords.begin(), it);

    std::string promotionString = "";

    // Finding the move from available moves
    for (Move move : board.moveList) {
        if (move.MoveFrom() == from && move.MoveTo() == to) {
            // Checking for promotion
            if (std::string(moveString).length() == 5) {
                int promotePiece = 0;

                // Checking for capture promotion
                if (board.GetPieceColor(from) != board.GetPieceColor(to)) {
                    switch (moveString[4])
                    {
                    case 'r':
                        promotePiece = rookPromoCapture;
                        break;
                    case 'q':
                        promotePiece = queenPromoCapture;
                        break;
                    case 'b':
                        promotePiece = bishopPromoCapture;
                        break;
                    case 'n':
                        promotePiece = knightPromoCapture;
                        break;
                    }
                } else {
                    switch (moveString[4])
                    {
                    case 'r':
                        promotePiece = rookPromotion;
                        break;
                    case 'q':
                        promotePiece = queenPromotion;
                        break;
                    case 'b':
                        promotePiece = bishopPromotion;
                        break;
                    case 'n':
                        promotePiece = knightPromotion;
                        break;
                    }
                }

                return Move(from, to, promotePiece);
            }

            return move;
        }
    }

    return Move();
}

void ParsePosition(Board &board, char* command) {
    // shifting pointer to where the token begins (skipping 'position' word)
    command += 9;

    // fen
    if (*command == 'f') {
        command += 4;

        std::string fen = "";

        while (*command && *(command + 1) != 'm') {
            fen += *command;
            command++;
        }
        // skipping space
        command++;

        board.SetByFen(fen.c_str());
    } else if (*command == 's') { // startpos
        command += 9;
    }

    // moves
    if (*command == 'm') {
        // skipping word 'moves'
        command += 5;

        // looping through the moves
        while (*command) {
            // skipping previous space
            command++;

            std::string moveString = "";

            bool multipleMoves = std::string(command).length() > 5;

            if (multipleMoves) {
                while (*command != ' ') {
                    moveString += *command;
                    command++;
                }

                board.MakeMove(ParseMove(board, moveString.c_str()));
                continue;
            } else {
                while (*command) {
                    moveString += *command;
                    command++;
                }
                board.MakeMove(ParseMove(board, moveString.c_str()));
            }
        }
    }
}