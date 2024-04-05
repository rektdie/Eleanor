#pragma once
#include <array>

constexpr short moveFromMask = 0x3F;
constexpr short moveToMask = 0xFC0;
constexpr short moveFlagsMask = 0xF000;

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
    uint16_t m_move;

    Move(){}

    Move(int from, int to, int flags) {
         m_move |= from;
         m_move |= to << 6;
         m_move |= flags << 12;
    }

    int MoveFrom() {
        return (m_move & moveFromMask);
    }

    int MoveTo() {
        return (m_move & moveToMask) >> 6;
    }

    int GetFlags() {
        return (m_move & moveFlagsMask) >> 12;
    }

    void PrintMove() {
        std::cout << squareCoords[MoveFrom()] << squareCoords[MoveTo()];
        std::cout << " (" << moveTypes[GetFlags()] << ")\n";
    }
};