#include "board.h"
#include "movegen.h"
#include "uci.h"

int main() {
	initLeaperAttacks();
	initSliderAttacks();

	Board board;

    UCILoop(board);

	return 0;
}
