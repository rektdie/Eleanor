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
        
        // Set from and to squares (6 bits each)
        viriMove |= MoveFrom();
        viriMove |= MoveTo() << 6;
        
        // Determine move type (bits 12-13)
        uint16_t moveType = 0;
        uint16_t promoPiece = 0;
        
        if (flags == kingCastle || flags == queenCastle) {
            moveType = 1; // Castling
        } 
        else if (flags == epCapture) {
            moveType = 2; // En passant
        } 
        else if (flags >= knightPromotion && flags <= queenPromoCapture) {
            moveType = 3; // Promotion
            // Set promo piece (bits 14-15)
            if (flags == knightPromotion || flags == knightPromoCapture) {
                promoPiece = 0; // Knight
            } 
            else if (flags == bishopPromotion || flags == bishopPromoCapture) {
                promoPiece = 1; // Bishop
            } 
            else if (flags == rookPromotion || flags == rookPromoCapture) {
                promoPiece = 2; // Rook
            } 
            else if (flags == queenPromotion || flags == queenPromoCapture) {
                promoPiece = 3; // Queen
            }
        }
        // else moveType remains 0 (normal move)
        
        viriMove |= moveType << 12;
        viriMove |= promoPiece << 14;
        
        return viriMove;
    }

    operator uint16_t() {
        return m_move;
    }
};
