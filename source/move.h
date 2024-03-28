#pragma once
#include "bitboards.h"

struct Move {
public:
    SQUARE moveFrom;
    SQUARE moveTo;

    Move(){}

    Move(const SQUARE from, const SQUARE to)
        : moveFrom(from), moveTo(to) {}

    void PrintMove();
};