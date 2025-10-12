#include "utils.h"
#include "movegen.h"

namespace UTILS {

// Zobrist //

U64 initialSeed = 0x60919C48E57863B9;

// PRNG using xorshift
static U64 RandomU64() {
    initialSeed ^= initialSeed << 13;
    initialSeed ^= initialSeed >> 7;
    initialSeed ^= initialSeed << 17;
    return initialSeed;
}

bool RandomBool() {
    return (RandomU64() & 1) == 1;
}

int RandomInt(int min, int max) {
    if (max < min) {
        int temp = min;
        min = max;
        max = temp;
    }

    int range = max - min + 1;

    return min + (RandomU64() % range);
}

void InitZobrist() {
    // init piece square keys 
    for (int i = White; i <= Black; i++) {
        for (int j = Pawn; j <= King; j++) {
            for (int k = a1; k <= h8; k++) {
                zKeys[i][j][k] = RandomU64();
            }
        } 
    }

    // init en passant file keys
    for (int i = 0; i < 8; i++) {
        zEnPassant[i] = RandomU64();
    }

    // init castling rights keys
    for (int i = 0; i < 16; i++) {
        zCastle[i] = RandomU64();
    }

    // init side key
    zSide = RandomU64();
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
    MOVEGEN::GenerateMoves<All>(board, true);

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

std::array<uint8_t, 16> CompressPieces(Board board) {
    const uint8_t UNMOVED_ROOK = 6;
    std::array<uint8_t, 16> compressed{};

    Bitboard occupancy = board.occupied;
    int index = 0;

    while (occupancy) {
        int square = occupancy.getLS1BIndex();
        int pieceType = board.GetPieceType(square);
        int pieceColor = board.GetPieceColor(square);
        int pieceCode = pieceType;

        if (pieceType == Rook) {
            if ((square == a1 && (board.castlingRights & whiteQueenRight))
                || (square == h1 && (board.castlingRights & whiteKingRight))
                || (square == a8 && (board.castlingRights & blackQueenRight))
                || (square == h8 && (board.castlingRights & blackKingRight)))
                pieceCode = UNMOVED_ROOK;
        }

        pieceCode |= (pieceColor << 3);  // Shift the color to the 3rd bit (bit 3)

        int byteIndex = index / 2;
        int bitShift = (index % 2) * 4;

        compressed[byteIndex] |= (pieceCode & 0x0F) << bitShift;

        index++;
        occupancy.PopBit(square);
    }

    return compressed;
}

int ConvertToWhiteRelative(Board &board, int score) {
    return board.sideToMove == White ? score : -score;
}

}
