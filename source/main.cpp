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

	// UCILoop(board);

	Perft(board, 10);

	return 0;
}
