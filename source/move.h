#pragma once

enum MOVE_FLAGS {
    quiet, doublePawnPush,
    kingCastle, queenCastle,
    capture, epCapture,
    knightPromotion, bishopPromotion, rookPromotion, queenPromotion,
    knightPromoCapture, bishopPromoCapture, rookPromoCapture, queenPromoCapture
};

constexpr std::array<const char*, 14> moveTypes {
    "quiet", "doublePawnPush",
    "kingCastle", "QueenCastle",
    "capture", "epCapture",
    "knightPromotion", "bishopPromotion", "rookPromotion", "queenPromotion",
    "knightPromoCapture", "bishopPromoCapture", "rookPromoCapture", "queenPromoCapture"
};

struct Move {
public:
    int moveFrom;
    int moveTo;
    uint16_t moveFlags;

    Move(){}

    Move(const int from, const int to, const uint16_t flags)
        : moveFrom(from), moveTo(to), moveFlags(flags) {}

    void PrintMove() {
	    std::cout << squareCoords[moveFrom] << squareCoords[moveTo];
        std::cout << '(' << moveTypes[moveFlags] << ")\n";
    }
};

struct LastMove {
public:
    int moveFrom;
    int moveTo;
    uint16_t moveFlags;

    int attackerPiece;
    int capturedPiece;
    int capturedColor;
    int enPassantTarget;
    std::array<bool, 4> castlingRights;

    LastMove(){}
    LastMove(const Move move, const int capturedPiece,
        const int capturedColor, const int enPassantTarget, 
        const std::array<bool, 4> castlingRights, int attackerPiece) {
        moveFrom = move.moveFrom;
        moveTo = move.moveTo;
        moveFlags = move.moveFlags;

        LastMove::capturedPiece = capturedPiece;
        LastMove::capturedColor = capturedColor;
        LastMove::enPassantTarget = enPassantTarget;
        LastMove::castlingRights = castlingRights;
        LastMove::attackerPiece = attackerPiece;
    }
};