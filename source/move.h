#pragma once
#include <array>
#include <iostream>
#include "types.h"

constexpr short moveFromMask = 0x3F;
constexpr short moveToMask = 0xFC0;

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
private:
    uint16_t m_move;
public:
    Move() {
        m_move = 0;
    }

    Move(int from, int to, int flags) {
        m_move = from;
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
        return m_move >> 12;
    }

    int IsCapture() {
        return (GetFlags() == capture || GetFlags() == epCapture
            || GetFlags() == knightPromoCapture || GetFlags() == bishopPromoCapture
            || GetFlags() == rookPromoCapture || GetFlags() == queenPromoCapture);
    }

    void PrintMove() {
        std::cout << squareCoords[MoveFrom()] << squareCoords[MoveTo()];

        int flags = GetFlags();

        if (flags == knightPromotion || flags == knightPromoCapture) {
            std::cout << 'n';
        } else if (flags == bishopPromotion || flags == bishopPromoCapture) {
            std::cout << 'b';
        } else if (flags == rookPromotion || flags == rookPromoCapture) {
            std::cout << 'r';
        } else if (flags == queenPromotion || flags == queenPromoCapture) {
            std::cout << 'q';
        }
    }

    operator uint16_t() {
        return m_move;
    }
};
