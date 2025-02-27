#pragma once
#include "board.h"
#include "uci.h"

extern int nodes;
extern bool benchStarted;

constexpr int invalidEntry = 111111;
constexpr int inf = 100000;


class SearchResults {
public:
    int score = 0;
    Move bestMove = Move();

    SearchResults(){}

    SearchResults(int pScore, Move pMove) {
        score = pScore;
        bestMove = pMove;
    }

    SearchResults(int pScore) {
        score = pScore;
    }
};

SearchResults PVS(Board board, int depth, int alpha, int beta);
void SearchPosition(Board &board, SearchParams params);
void StopSearch();

constexpr int moveScoreTable[6][6] = {
    105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600
};

void ListScores(Board &board);
