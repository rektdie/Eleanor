#include <iostream>
#include "types.h"
#include "board.h"
#include "movegen.h"
#include "perft.h"

int main() {
	initLeaperAttacks();
	initSliderAttacks();

	Board board;
	board.SetByFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
	Perft(board, 5);

	return 0;
}