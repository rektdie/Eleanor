#include <iostream>
#include "types.h"
#include "board.h"
#include "movegen.h"
#include "perft.h"
#include "uci.h"

int main() {
	initLeaperAttacks();
	initSliderAttacks();

	Board board;

	ParsePosition(board, "position startpos moves a2a4 b7b6");
	board.PrintBoard();

	return 0;
}