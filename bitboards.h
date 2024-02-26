#pragma once
#include "types.h"

struct Bitboard {
private:
	U64 m_board;
public:
	Bitboard()
		: m_board(0ULL) {}

	Bitboard(const U64& board)
		: m_board(board) {}
	
	bool IsSet(int square) const;
	void PrintBoard() const;

	static Bitboard GetSquare(int square) {
		return (1ULL << square);
	};

	Bitboard operator&(const Bitboard& other) const;
	Bitboard operator|(const Bitboard& other) const;
	Bitboard operator^(const Bitboard& other) const;
	Bitboard operator*(const Bitboard& other) const;

	Bitboard operator&=(const Bitboard& other);
	Bitboard operator|=(const Bitboard& other);
	Bitboard operator^=(const Bitboard& other);
	Bitboard operator*=(const Bitboard& other);

	Bitboard operator<<(int other);
	Bitboard operator>>(int other);
	Bitboard operator~();
};