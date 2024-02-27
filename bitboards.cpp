#include <iostream>
#include "bitboards.h"
#include <bitset>

void Bitboard::PrintBoard() const {
	for (int rank = 7; rank >= 0; rank--) {
            
            std::cout << "+---+---+---+---+---+---+---+---+\n";
		    std::cout << "| ";
		    
            for (int file = 0; file < 8; file++) {
                int square = rank * 8 + file;
                
                std::cout << IsSet << " | ";
            }
            std::cout << ' ' << rank + 1<< '\n';
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
