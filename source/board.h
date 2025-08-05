#pragma once
#include <array>
#include "bitboard.h"
#include "types.h"
#include "move.h"
#include "accumulator.h"

constexpr int nullPieceType = 100;
constexpr int noEPTarget = -1;

constexpr int MAX_MOVES = 218;

class Board {
public:
	Board() {
		SetByFen(StartingFen);
	}

	std::array<Bitboard, 6> pieces;
	std::array<Bitboard, 2> colors;

	Bitboard occupied;

    ACC::AccumulatorPair accPair;

    std::array<Bitboard, 6> pieceThreats;
	std::array<Bitboard, 2> colorThreats;

    std::array<Move, MAX_MOVES> moveList;
    int currentMoveIndex = 0;

	bool sideToMove = White;

    uint8_t castlingRights = 0;
	int enPassantTarget = noEPTarget;

	short positionIndex = 0;

	int halfMoves = 0;
	int fullMoves = 1;

    U64 hashKey = 0ULL;
    U64 pawnKey = 0ULL;

	void Reset();
	void SetByFen(std::string_view fen);
	std::string GetFen();
	void PrintBoard();

	void PrintNNUE();

	void AddMove(Move move);
	void ResetMoves();
	void ListMoves();

	void ResetAccPair();

	int GetPieceType(int square);
	int GetPieceColor(int square);

	bool InCheck();

	void SetPiece(int piece, int square, bool color);
	void RemovePiece(int piece, int square, bool color);

	void Promote(int square, int pieceType, int color, bool isCapture);
	bool MakeMove(Move move);

    bool InPossibleZug();

    Bitboard AttacksTo(int square, Bitboard occupancy);
};
