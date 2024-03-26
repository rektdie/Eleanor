#pragma once
#include <array>
#include "bitboards.h"
#include "types.h"
#include "movegen.h"
#include <vector>

class Board {
private:
	std::vector<Move> moveList;
public:
	Board() {
		Init();
	}

	std::array<Bitboard, 6> pieces;
	std::array<Bitboard, 2> colors;
	Bitboard occupied;

	COLOR sideToMove = White;
	int halfMoves = 0;
	int fullMoves = 0;

	Move lastMove;

	std::array<bool, 4> castlingRights = { false, false, false, false };
	int enPassantTarget;

	void Init();
	void Reset();
	void SetByFen(const char* fen);
	void PrintBoard();

	void AddMove(Move move);
	void ResetMoves();
	void ListMoves();

	int GetPieceType(int square);
	int GetPieceColor(int square);

	bool InCheck(bool side);

	void DoMove(Move move);
};