#pragma once
#include "board.h"
#include "utils.h"

namespace DATAGEN {

constexpr int SOFT_NODES = 5000;
constexpr int HARD_NODES = 100000;
constexpr int RAND_MOVES = 8;

struct MarlinFormat {
    uint64_t occupancy;      // 8 bytes: Bitboard representing all occupied squares (includes all pieces)
    std::array<uint8_t, 16> pieces; // 16 bytes: 64 squares stored in 4-bit format (2 squares per byte)
    uint8_t stmEPSquare;     // 1 byte: STM (side to move) in bit 7, EP square in bits 0-5
    uint8_t halfmoveClock;   // 1 byte: Halfmove counter for the 50-move rule (incremented after each move)
    uint16_t fullmoveNumber; // 2 bytes: Full move counter, starting at 1 (increments after Black's move)
    int16_t eval;            // 2 bytes: Search score (e.g., centipawns, positive = White advantage)
    uint8_t wdl;             // 1 byte: 0 = white loss, 1 = draw, 2 = white win
    uint8_t extra;           // 1 byte: Reserved for future use or additional data

    void packFrom(Board& board, int16_t eval, uint8_t wdl) {
        occupancy = board.occupied;
        pieces = UTILS::CompressPieces(board);
        uint8_t stm_ep_square = (board.sideToMove<< 7) | (board.enPassantTarget & 0x3F);
        halfmoveClock = board.halfMoves;
        fullmoveNumber = board.fullMoves;
        this->eval = eval;
        this->wdl = wdl;
        extra = 0;
    }
};

struct ScoredMove {
    uint16_t move;
    int16_t score;

    ScoredMove(Move m, int s) {
        move = m;
        score = s;
    }
};

struct Game {
    MarlinFormat format;
    std::vector<ScoredMove> moves;

    Game(MarlinFormat f, std::vector<ScoredMove> m) {
        format = f;
        moves = m;
    }
};

void Run();
}