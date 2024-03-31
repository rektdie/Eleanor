#pragma once
#include <array>
#include "bitboards.h"
#include "types.h"
#include "move.h"
#include <vector>

class Board {
public:
	Board() {
		Init();
	}

	std::array<Bitboard, 6> pieces;
	std::array<Bitboard, 2> colors;
	Bitboard occupied;

	std::vector<Move> moveList;

	bool sideToMove = White;
	int halfMoves = 0;
	int fullMoves = 0;

	LastMove lastMove;

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

	void Promote(int square, int pieceType, int color, bool isCapture);
	void DoMove(Move move);
	void UnmakeMove();
};