#include <iostream>
#include "bitboards.h"
#include <bitset>

void Bitboard::PrintBoard() const {
	Bitboard mask = 0xff00000000000000;

	for (int i = 0; i < 8; i++) {
		std::bitset<8> currentRank = ((this->m_board & mask.m_board) >> (7 - i) * 8);

		std::cout << "+---+---+---+---+---+---+---+---+\n";
		std::cout << "| ";
		for (int j = 0; j < 8; j++) {
			std::cout << currentRank[j] << " | ";
		}
		std::cout << ' ' << 8 - i << '\n';
		mask.m_board = (mask.m_board >> 8);
	}
	std::cout << "+---+---+---+---+---+---+---+---+\n";
	std::cout << "  a   b   c   d   e   f   g   h\n\n";
}

bool Bitboard::IsSet(int square) const {
	return (1ULL << square) & this->m_board;
}

// Operator overloading for easier comparisons
Bitboard Bitboard::operator&(const Bitboard& other) const {
	return this->m_board & other.m_board;
}
Bitboard Bitboard::operator|(const Bitboard& other) const {
	return this->m_board | other.m_board;
}
Bitboard Bitboard::operator^(const Bitboard& other) const {
	return this->m_board ^ other.m_board;
}
Bitboard Bitboard::operator*(const Bitboard& other) const {
	return this->m_board * other.m_board;
}

Bitboard Bitboard::operator&=(const Bitboard& other) {
	return this->m_board &= other.m_board;
}
Bitboard Bitboard::operator|=(const Bitboard& other) {
	return this->m_board |= other.m_board;
}
Bitboard Bitboard::operator^=(const Bitboard& other) {
	return this->m_board ^= other.m_board;
}
Bitboard Bitboard::operator*=(const Bitboard& other) {
	return this->m_board *= other.m_board;
}

Bitboard Bitboard::operator<<(int other) {
	return this->m_board << other;
}
Bitboard Bitboard::operator>>(int other) {
	return this->m_board >> other;
}

Bitboard Bitboard::operator~() {
	return ~(this->m_board);
}