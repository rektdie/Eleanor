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
    
	bool IsSet(int square) const;
	void SetBit(int square);
	void PopBit(int square);
	void PrintBoard() const;

	int PopCount() const;
	int getLS1BIndex() const;
	static Bitboard getOccupancy(int index, Bitboard attackMask);

    static Bitboard GetSquare(int square) {
		return (1ULL << square);
	};

	Bitboard operator&(const Bitboard& other) const;
	Bitboard operator|(const Bitboard& other) const;
	Bitboard operator^(const Bitboard& other) const;
	Bitboard operator*(const Bitboard& other) const;
    
	Bitboard operator&(const uint64_t& other) const;
	Bitboard operator|(const uint64_t& other) const;
	Bitboard operator^(const uint64_t& other) const;
	Bitboard operator*(const uint64_t& other) const;

	Bitboard operator&=(const Bitboard& other);
	Bitboard operator|=(const Bitboard& other);
	Bitboard operator^=(const Bitboard& other);

	Bitboard operator&=(const uint64_t& other);
	Bitboard operator|=(const uint64_t& other);
	Bitboard operator^=(const uint64_t& other);

	Bitboard operator<<(int other);
	Bitboard operator>>(int other);
	Bitboard operator~();
    operator uint64_t();
};
