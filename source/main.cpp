#include <iostream>
#include "types.h"
#include "board.h"
#include "movegen.h"
#include "perft.h"

int main() {
	initLeaperAttacks();
	initSliderAttacks();

	Board board;
	board.PrintBoard();
	Perft(board, 3);

	return 0;
}