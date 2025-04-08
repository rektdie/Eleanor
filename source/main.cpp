#include "board.h"
#include "movegen.h"
#include "uci.h"
#include "utils.h"
#include "benchmark.h"
#include "search.h"
#include "datagen.h"
#include "nnue.h"

int main(int argc, char* argv[]) {
	MOVEGEN::initLeaperAttacks();
	MOVEGEN::initSliderAttacks();
    UTILS::InitZobrist();
    SEARCH::InitLMRTable();

    NNUE::net.Load("beans.bin");

	Board board;

    if (argc > 1) {
        if (std::string(argv[1]) == "bench") {
            RunBenchmark();
        } else if (std::string(argv[1]) == "datagen") {
            int positions = 1;
            int threads = 1;
            if (argc > 2) {
                positions = std::stoi(argv[2]);

                if (argc > 3) {
                    threads = std::stoi(argv[3]);
                }
            }
            DATAGEN::Run(positions, threads);
        }
    } else {
        UCILoop(board);
    }

	return 0;
}
