#include <iostream>
#include <fstream>
#include <cassert>
#include "tests.h"
#include "utils.h"
#include "search.h"

namespace TEST {

void SEE() {
	std::ifstream file("tests/SEE.txt");
	std::string line;

	while (std::getline(file, line)) {
		std::stringstream iss(line);
		std::cout << line << ' ';
		Board board;

		std::vector tokens = UTILS::split(line, '|');

		std::string fen = tokens[0];
		board.SetByFen(fen);

		// Trim whitespaces
		tokens[1].erase(0, tokens[1].find_first_not_of(" \t\r\n"));
		tokens[1].erase(tokens[1].find_last_not_of(" \t\r\n") + 1);

		tokens[2].erase(0, tokens[2].find_first_not_of(" \t\r\n"));
		tokens[2].erase(tokens[2].find_last_not_of(" \t\r\n") + 1);

		Move move = UTILS::parseMove(board, tokens[1]);

		int gain = std::stoi(tokens[2]);

		assert(SEARCH::SEE(board, move, gain - 1));
        assert(SEARCH::SEE(board, move, gain));
        assert(!SEARCH::SEE(board, move, gain + 1));

        std::cout << "| PASSED" << std::endl;
	}
}

}