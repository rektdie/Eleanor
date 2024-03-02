#include <iostream>
#include "types.h"
#include "board.h"
#include "movegen.h"

int main() {
	Board board;
	board.Init();

	initLeaperAttacks();

	system("Color 0a");
	maskBishopAttacks(e6).PrintBoard();

	return 0;
}