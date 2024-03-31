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