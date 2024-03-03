#pragma once
#include <iostream>
#include "types.h"

struct Bitboard {
private:
	U64 m_board;
public:
	Bitboard()
		: m_board(0ULL) {}

	Bitboard(const U64& board)
		: m_board(board) {}

	U64 GetBoard() const;
	bool IsSet(int square) const;
	void SetBit(int square);
	void PrintBoard() const;

	int popCount() const;

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