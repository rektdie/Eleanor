#pragma once
#include <array>
#include "bitboard.h"
#include "types.h"
#include "move.h"

constexpr int nullPieceType = 100;
constexpr int noEPTarget = -1;

inline int positionIndex = 0;
inline U64 positionHistory[1000];

class Board {
public:
	Board() {
		SetByFen(StartingFen);
	}

	std::array<Bitboard, 6> pieces;
	std::array<Bitboard, 2> colors;

	Bitboard occupied;

    std::array<std::array<U64, 64>, 2> threatMaps;

    std::array<Move, 218> moveList;
    int currentMoveIndex = 0;

	bool sideToMove = White;

    uint8_t castlingRights = 0;
	int enPassantTarget = noEPTarget;

    U64 hashKey = 0ULL;

	void Init();
	void Reset();
	void SetByFen(std::string_view fen);
	void PrintBoard();

	void AddMove(Move move);
	void ResetMoves();
	void ListMoves();

    U64 GetThreatMaps(bool side);
	int GetPieceType(int square);
	int GetPieceColor(int square);
	
	bool InCheck();
	bool IsLegal(Move move);

	void SetPiece(int piece, int square, bool color);
	void RemovePiece(int piece, int square, bool color);

	void Promote(int square, int pieceType, int color, bool isCapture);
	void MakeMove(Move move);

    bool InPossibleZug();
};
