#include "board.h"
#include "movegen.h"
#include "uci.h"
#include "utils.h"
#include "benchmark.h"
#include "search.h"
#include "datagen.h"

int main(int argc, char* argv[]) {
	MOVEGEN::initLeaperAttacks();
	MOVEGEN::initSliderAttacks();
    UTILS::InitZobrist();
    SEARCH::InitLMRTable();

	Board board;

    if (argc > 1) {
        if (std::string(argv[1]) == "bench") {
            RunBenchmark();
        } else if (std::string(argv[1]) == "datagen") {
            int count = 1;
            if (argc > 2) {
                count = std::stoi(argv[2]);
            }
            DATAGEN::Run(count);
        }
    } else {
        UCILoop(board);
    }

	return 0;
}
