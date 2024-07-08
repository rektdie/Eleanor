#include "board.h"
#include "movegen.h"
#include "uci.h"
#include "tt.h"

int main() {
	initLeaperAttacks();
	initSliderAttacks();
    InitZobrist();

	Board board;

    UCILoop(board);

	return 0;
}
