#include <iostream>
#include "types.h"
#include "board.h"
#include "movegen.h"

int main() {
	Board board;
	board.Init();

	initLeaperAttacks();
	rookAttacksOnTheFly(e4, board.occupied).PrintBoard();

	return 0;
}