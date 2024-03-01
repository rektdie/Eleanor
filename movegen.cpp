#include "movegen.h"

Bitboard pawnAttacks[2][64];

Bitboard maskPawnAttacks(int side, int square) {
	Bitboard attacks;
	Bitboard attacker;

	Bitboard notAfile = ~files[A];
	Bitboard notHfile = ~files[H];
	Bitboard notABfile = notAfile & ~files[B];
	Bitboard notHGfile = notHfile & ~files[G];

	attacker.SetBit(square);

	// White
	if (!side) {
		if (((attacker << noEa) & notAfile).GetBoard()) attacks |= (attacker << noEa);
		if (((attacker << noWe) & notHfile).GetBoard()) attacks |= (attacker << noWe);
	}
	// Black
	else {
		if (((attacker << soEa) & notAfile).GetBoard()) attacks |= (attacker << soEa);
		if (((attacker << soWe) & notHfile).GetBoard()) attacks |= (attacker << soWe);
	}
	return attacks;
}

void initLeaperAttacks() {
	for (int square = 0; square < 64; square++) {
		pawnAttacks[White][square] = maskPawnAttacks(White, square);
		pawnAttacks[Black][square] = maskPawnAttacks(Black, square);
	}
}