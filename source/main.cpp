#include <iostream>
#include "types.h"
#include "board.h"
#include "movegen.h"
#include "perft.h"

int main() {
	initLeaperAttacks();
	initSliderAttacks();

	Board board;
	board.SetByFen("r4rk1/1pp1qppp/p1npBn2/2b1p1B1/4P1b1/P1NP1N2/1PP1QPPP/R4RK1 b - - 1 10");
	Perft(board, 3);

	return 0;
}