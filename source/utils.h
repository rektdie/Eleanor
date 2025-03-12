#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <sstream>
#include "move.h"
#include "board.h"

std::vector<std::string> split(std::string_view str, char delim);

int parseSquare(std::string_view str);

Move parseMove(Board &board, std::string_view str);
