#pragma once
#include <array>
#include <string_view>
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

constexpr std::array<std::string_view, 14> moveTypes {
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

    bool IsCapture() {
        return (GetFlags() == capture || GetFlags() == epCapture
            || GetFlags() == knightPromoCapture || GetFlags() == bishopPromoCapture
            || GetFlags() == rookPromoCapture || GetFlags() == queenPromoCapture);
    }

    bool IsQuiet() {
        return (GetFlags() == quiet || GetFlags() == kingCastle
        || GetFlags() == queenCastle || GetFlags() == doublePawnPush);
    }

    bool IsPromo() {
        return (GetFlags() >= knightPromotion);
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

    uint16_t ConvertToViriMoveFormat() {
        uint16_t viriMove = 0;
        int flags = GetFlags();
    
        viriMove = MoveFrom();

        if (flags == kingCastle || flags == queenCastle) {
            uint16_t moveTo = 0;
            bool side = (MoveFrom() / 8 == 0) ? White : Black;

            if (flags == kingCastle) {
                moveTo = (side == White) ? h1 : h8;
            } else {
                moveTo = (side == White) ? a1 : a8;
            }

            viriMove |= (moveTo << 6);

            viriMove |= 0x8000;

            return viriMove;
        }

        viriMove |= (MoveTo() << 6);

        if (flags == epCapture) {
            viriMove |= 0x4000;
        } else if (flags >= knightPromotion && flags <= queenPromoCapture) {
            uint16_t promoPiece = 0;

            if (flags == knightPromotion || flags == knightPromoCapture) {
                promoPiece = 0;
            } else if (flags == bishopPromotion || flags == bishopPromoCapture) {
                promoPiece = 1;
            } else if (flags == rookPromotion || flags == rookPromoCapture) {
                promoPiece = 2;
            } else if (flags == queenPromotion || flags == queenPromoCapture) {
                promoPiece = 3;
            }

            viriMove |= 0xC000;
            viriMove |= (promoPiece << 12);
        }

        return viriMove;
    }

    operator uint16_t() {
        return m_move;
    }
};
