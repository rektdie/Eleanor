#include <iostream>
#include "types.h"
#include "board.h"
#include "movegen.h"

int main() {
	Board board;
	board.Init();

	initLeaperAttacks();
	maskRookAttacks(e4).PrintBoard();

	return 0;
}