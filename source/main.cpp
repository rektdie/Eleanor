#include "board.h"
#include "movegen.h"
#include "uci.h"
#include "tt.h"
#include "benchmark.h"

int main(int argc, char* argv[]) {
	MOVEGEN::initLeaperAttacks();
	MOVEGEN::initSliderAttacks();
    InitZobrist();

	Board board;

    if (argc > 1) {
        if (std::string(argv[1]) == "bench") {
            RunBenchmark();
        }
    } else {
        UCILoop(board);
    }

	return 0;
}
