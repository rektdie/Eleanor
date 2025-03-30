#include "utils.h"
#include "movegen.h"

namespace UTILS {

// Zobrist //

U64 initialSeed = 0x60919C48E57863B9;

// PRNG using xorshift
static U64 RandomNum() {
    initialSeed ^= initialSeed << 13;
    initialSeed ^= initialSeed >> 7;
    initialSeed ^= initialSeed << 17;
    return initialSeed;
}

void InitZobrist() {
    // init piece square keys 
    for (int i = White; i <= Black; i++) {
        for (int j = Pawn; j <= King; j++) {
            for (int k = a1; k <= h8; k++) {
                zKeys[i][j][k] = RandomNum();
            }
        } 
    }

    // init en passant file keys
    for (int i = 0; i < 8; i++) {
        zEnPassant[i] = RandomNum();
    }

    // init castling rights keys
    for (int i = 0; i < 16; i++) {
        zCastle[i] = RandomNum();
    }

    // init side key
    zSide = RandomNum();
}

U64 GetHashKey(Board &board) {
    U64 key = 0ULL;

    // XORing pieces
    for (int color = White; color <= Black; color++) {
        for (int piece = Pawn; piece <= King; piece++) {
            Bitboard pieceSet = board.pieces[piece] & board.colors[color];

            while (pieceSet) {
                int square = pieceSet.getLS1BIndex();

                key ^= zKeys[color][piece][square];

                pieceSet.PopBit(square);
            }
        }
    }

    // XORing en passant
    if (board.enPassantTarget != -1) {
        key ^= zEnPassant[board.enPassantTarget % 8];
    }

    // XORing castling rights
    key ^= zCastle[board.castlingRights];

    // XORing side to move
    if (board.sideToMove) key ^= zSide;

    return key;
}

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
    MOVEGEN::GenerateMoves<All>(board);

    int from = parseSquare(str.substr(0, 2));
    int to = parseSquare(str.substr(2, 2));

    int flag = 0;

    // Promotion
    if (str.length() > 4) {
        if (board.occupied.IsSet(to)) {
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

std::array<uint8_t, 32> CompressPieces(Board &board) {
    const uint8_t UNMOVED_ROOK = 0x10;  // Special value for unmoved rooks

    std::array<uint8_t, 32> compressed;

    for (int square = a1; square <= h8; square++) {
        int pieceCode = 0;

        for (int pieceType = Pawn; pieceType <= King; pieceType++) {
            Bitboard pieceBitboard = board.pieces[pieceType];

            if (pieceBitboard.IsSet(square)) {
                if (board.colors[White] & board.pieces[pieceType]) {
                    pieceCode = pieceType + 1;  // White pieces: 1-6
                } else {
                    pieceCode = pieceType + 7;  // Black pieces: 7-12
                }

                // If it's a rook and hasn't moved (check castling rights)
                if (pieceType == Rook) {
                    if (board.sideToMove == White) {
                        // If the white rooks are still eligible for castling, they haven't moved
                        if ((board.castlingRights & (whiteKingRight | whiteQueenRight)) != 0) {
                            pieceCode |= UNMOVED_ROOK;  // Mark as unmoved rook
                        }
                    } else if (board.sideToMove == Black) {
                        // If the black rooks are still eligible for castling, they haven't moved
                        if ((board.castlingRights & (blackKingRight | blackQueenRight)) != 0) {
                            pieceCode |= UNMOVED_ROOK;  // Mark as unmoved rook
                        }
                    }
                }
                break;
            }
        }

        // Determine the byte index for storing the piece code (since we are packing 2 pieces per byte)
        int byteIndex = square / 2;

        // Pack the piece code into the byte array
        if (square % 2 == 0) {
            compressed[byteIndex] = (compressed[byteIndex] & 0xF0) | (pieceCode & 0x0F);
        } else {
            compressed[byteIndex] = (compressed[byteIndex] & 0x0F) | ((pieceCode & 0x0F) << 4);
        }
    }

    return compressed;
}

}
