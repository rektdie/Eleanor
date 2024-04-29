#include <iostream>
#include "types.h"
#include "board.h"
#include "movegen.h"
#include "perft.h"
#include "evaluate.h"
#include "search.h"

int main() {
	initLeaperAttacks();
	initSliderAttacks();

	Board board;

	SearchPosition(board, 3);

	return 0;
}