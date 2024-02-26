#pragma once
#include "bitboards.h"
#include <array>

class Board {
public:
	std::array<Bitboard, 6> pieces;
	std::array<Bitboard, 2> colors;
	Bitboard occupied;

	bool sideToMove = White;
	int halfMoves = 0;
	int fullMoves = 0;

	std::array<bool, 4> castlingRights = { false, false, false, false };
	Bitboard enPassantTarget;

	void Init();
	void Reset();
	void SetByFen(const char* fen);
};