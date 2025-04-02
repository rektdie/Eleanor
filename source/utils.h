#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <sstream>
#include "move.h"
#include "board.h"

namespace UTILS {

// Zobrist //

// 2[colors] * 6[pieces] * 64[squares]
inline U64 zKeys[2][6][64];

// There are 8 files
inline U64 zEnPassant[8];

// 16 possible castling right variations
inline U64 zCastle[16];

inline U64 zSide;

void InitZobrist();
U64 GetHashKey(Board &board);

std::vector<std::string> split(std::string_view str, char delim);

int parseSquare(std::string_view str);

Move parseMove(Board &board, std::string_view str);

std::array<uint8_t, 16> CompressPieces(Board board);

}
