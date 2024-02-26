#include <iostream>
#include "types.h"
#include "board.h"

int main() {
	Board board;
	board.Init();

	// Printing the white pawns
	(board.colors[White] & board.pieces[Pawn]).PrintBoard();

	return 0;
}