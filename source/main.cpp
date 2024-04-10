#include <iostream>
#include "types.h"
#include "board.h"
#include "movegen.h"
#include "perft.h"

int main() {
	initLeaperAttacks();
	initSliderAttacks();

	Board board;
	board.SetByFen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1R1K b kq - 1 1");
	board.PrintBoard();
	Perft(board, 2);

	return 0;
}