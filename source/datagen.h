#pragma once
#include "board.h"
#include "utils.h"

struct PackedBoard {
    uint64_t occupancy;      // 8 bytes: Bitboard representing all occupied squares (includes all pieces)
    std::array<uint8_t, 32> pieces; // 32 bytes: 64 squares stored in 4-bit format (2 squares per byte)
    uint8_t sideToMove;      // 1 byte: 0 = White to move, 1 = Black to move
    uint8_t castlingRights;  // 1 byte: 4-bit bitmask indicating available castling rights
    uint8_t enPassantTarget; // 1 byte: En passant target square (0-63) or 64 if no en passant is available
    uint8_t halfmoveClock;   // 1 byte: Halfmove counter for the 50-move rule (incremented after each move)
    uint16_t fullmoveNumber; // 2 bytes: Full move counter, starting at 1 (increments after Black's move)
    int16_t eval;            // 2 bytes: Static evaluation score (e.g., centipawns, positive = White advantage)
    uint8_t wdl;             // 1 byte: Win-Draw-Loss classification (for training or result storage)
    uint8_t extra;           // 1 byte: Reserved for future use or additional data

    void packFrom(Board& board, int16_t eval, uint8_t wdl) {
        occupancy = board.occupied;
        pieces = UTILS::CompressPieces(board);
        sideToMove = board.sideToMove;
        castlingRights = board.castlingRights;
        enPassantTarget = (board.enPassantTarget == noEPTarget) ? 64 : board.enPassantTarget;
        halfmoveClock = board.halfMoves;
        fullmoveNumber = board.fullMoves;
        this->eval = eval;
        this->wdl = wdl;
        extra = 0;
    }
};
