#include <iostream>
#include "types.h"
#include "board.h"
#include "movegen.h"

int main() {
	initLeaperAttacks();
	initSliderAttacks();

	Board board;
	GenerateMoves(board);

	board.ListMoves();

	return 0;
}