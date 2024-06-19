#include "board.h"
#include "movegen.h"
#include "uci.h"
#include "perft.h"

int main() {
	initLeaperAttacks();
	initSliderAttacks();

	Board board;

    Perft(board, 7);

	return 0;
}
