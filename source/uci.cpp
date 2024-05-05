#include "uci.h"
#include <iostream>
#include <algorithm>
#include <string>
#include "types.h"
#include "movegen.h"
#include "search.h"

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

void ParsePosition(Board &board, const char* command) {
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
        board.SetByFen(StartingFen);
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

void ParseGo(Board &board, const char* command) {
    // init depth
    int depth = -1;

    // handle fixed depth search
    if (std::string(command).find("depth") != std::string::npos) {
        // skipping to the depth number
        command += 9;

        depth = atoi(command);
    }

    // search position
    SearchPosition(board, depth);
}

void UCILoop(Board &board) {
    // reset STDIN & STDOUT buffers
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    // define user / GUI input buffer
    std::string input = "";

    // print engine info
    std::cout << "id name Eleanor\n";
    std::cout << "id name rektdie\n";
    std::cout << "uciok\n";

    // main loop
    while (true) {
        // reset user / GUI input
        getline(std::cin, input);

        // make sure output reaches the GUI
        fflush(stdout);

        // make sure input is available
        if (input[0] == '\n') {
            continue;
        }

        // parse UCI "isready" command
        if (input.find("isready") != std::string::npos) {
            std::cout << "readyok\n";
            continue;
        }

        // parse UCI "position" command
        if (input.find("position") != std::string::npos) {
            ParsePosition(board, input.c_str());
            continue;
        }

        // parse UCI "ucinewgame" command
        if (input.find("ucinewgame") != std::string::npos) {
            ParsePosition(board, "position startpos");
            continue;
        }

        // parse UCI "go" command
        if (input.find("go") != std::string::npos) {
            ParseGo(board, input.c_str());
            continue;
        }

        // parse UCI "quit" command
        if (input.find("quit") != std::string::npos) {
            // stop the loop
            break;
        }

        // parse UCI "uci" command
        if (input.find("uci") != std::string::npos) {
            // print engine info
            std::cout << "id name Eleanor\n";
            std::cout << "id name rektdie\n";
            std::cout << "uciok\n";
            continue;
        }
    }
}