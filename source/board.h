#pragma once
#include <array>
#include "bitboards.h"
#include "types.h"
#include "move.h"

class Board {
public:
	Board() {
		Init();
	}

	std::array<Bitboard, 6> pieces;
	std::array<Bitboard, 2> colors;

	Bitboard occupied;

    std::array<Move, 218> moveList;
    int currentMoveIndex = 0;

	bool sideToMove = White;
	int halfMoves = 0;
	int fullMoves = 0;

    uint8_t castlingRights = 0;
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

	void SetPiece(int piece, int square, bool color);
	void RemovePiece(int piece, int square, bool color);

	void Promote(int square, int pieceType, int color, bool isCapture);
	void MakeMove(Move move);
};
