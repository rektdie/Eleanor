#include <iostream>
#include "types.h"
#include "board.h"
#include "movegen.h"

int main() {
	Board board;
	board.Init();

	initLeaperAttacks();

	pawnAttacks[White][e4].PrintBoard();

	return 0;
}