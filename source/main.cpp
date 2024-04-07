#include <iostream>
#include "types.h"
#include "board.h"
#include "movegen.h"
#include "perft.h"

int main() {
	initLeaperAttacks();
	initSliderAttacks();

	Board board;
	board.PrintBoard();
	GenerateMoves(board, board.sideToMove);
	board.ListMoves();

	return 0;
}