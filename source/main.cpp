#include <iostream>
#include "types.h"
#include "board.h"
#include "movegen.h"
#include "perft.h"
#include "evaluate.h"

int main() {
	initLeaperAttacks();
	initSliderAttacks();

	Board board;

	return 0;
}