#include "bitboards.h"

void Bitboard::PrintBoard() const {
	for (int rank = 7; rank >= 0; rank--) {

		std::cout << "+---+---+---+---+---+---+---+---+\n";
		std::cout << "| ";

		for (int file = 0; file < 8; file++) {
			int square = rank * 8 + file;

			std::cout << IsSet(square) << " | ";
		}
		std::cout << ' ' << rank + 1 << '\n';
	}

	std::cout << "+---+---+---+---+---+---+---+---+\n";
	std::cout << "  a   b   c   d   e   f   g   h\n\n";
}

U64 Bitboard::GetBoard() const {
	return m_board;
}

bool Bitboard::IsSet(int square) const {
	return (1ULL << square) & m_board;
}

void Bitboard::SetBit(int square) {
	m_board |= (1ULL << square);
}

void Bitboard::PopBit(int square) {
	if (Bitboard(m_board).IsSet(square)) {
		m_board &= (~Bitboard::GetSquare(square)).GetBoard(); 
	}
}

int Bitboard::popCount() const {
	U64 board = m_board;
	int count = 0;

	while (board) {
		board &= board - 1;

		count++;
	}

	return count;
}

// LS1B = least significant first bit
int Bitboard::getLS1BIndex() const {
	if (m_board) {
		// count trailing bits before LS1B
		return Bitboard((m_board & -m_board) - 1).popCount();
	}

	return -1;
}

Bitboard Bitboard::getOccupancy(int index, Bitboard attackMask) {
	Bitboard occupancy;
	int squareNum = attackMask.popCount();

	for (int count = 0; count < squareNum; count++) {
		int square = attackMask.getLS1BIndex();
		
		attackMask.PopBit(square);

		if (index & (1 << count)) {
			occupancy |= (1ULL << square);
		}
	}

	return occupancy;
}

// Operator overloading for easier comparisons
Bitboard Bitboard::operator&(const Bitboard& other) const {
	return m_board & other.m_board;
}
Bitboard Bitboard::operator|(const Bitboard& other) const {
	return m_board | other.m_board;
}
Bitboard Bitboard::operator^(const Bitboard& other) const {
	return m_board ^ other.m_board;
}
Bitboard Bitboard::operator*(const Bitboard& other) const {
	return m_board * other.m_board;
}

Bitboard Bitboard::operator&=(const Bitboard& other) {
	return m_board &= other.m_board;
}
Bitboard Bitboard::operator|=(const Bitboard& other) {
	return m_board |= other.m_board;
}
Bitboard Bitboard::operator^=(const Bitboard& other) {
	return m_board ^= other.m_board;
}

Bitboard Bitboard::operator<<(int other) {
	if (other > 0) {
		return m_board << other;
	}
	else {
		return m_board >> other * -1;
	}
}
Bitboard Bitboard::operator>>(int other) {
	if (other > 0) {
		return m_board >> other;
	}
	else {
		return m_board << other * -1;
	}
}

Bitboard Bitboard::operator~() {
	return ~(m_board);
}