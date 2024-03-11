#include <iostream>
#include "types.h"
#include "board.h"
#include "movegen.h"

int main() {
	initLeaperAttacks();
	
	Board board;
	board.Init();

	Bitboard::getOccupancy(4095, maskRookAttacks(e5)).PrintBoard();

	return 0;
}