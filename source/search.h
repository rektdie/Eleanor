#pragma once
#include "board.h"
#include "uci.h"
#include <vector>
#include "move.h"
#include <algorithm>

namespace SEARCH {

constexpr int MATE_SCORE = -99000;
constexpr int MAX_DEPTH = 64;
constexpr int MAX_HISTORY = 16384;

extern U64 nodes;
extern bool benchStarted;

constexpr int inf = 100000;

inline bool searchStopped = false;

//                   [id][ply]
inline int killerMoves[2][64];

inline int lmrTable[MAX_DEPTH+1][MAX_MOVES];

void InitLMRTable();

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

class History {
private:
    int historyMoves[2][64][64];
public:
    void Update(bool stm, Move move, int bonus) {
        int clampedBonus = std::clamp(bonus, -MAX_HISTORY, MAX_HISTORY);
        historyMoves[stm][move.MoveFrom()][move.MoveTo()] += 
            clampedBonus - historyMoves[stm][move.MoveFrom()][move.MoveTo()] * std::abs(clampedBonus) / MAX_HISTORY;
    }

    void Clear() {
        for (auto &side : historyMoves) {
            for (auto &from : side) {
                for (auto &to : from) {
                    to = 0;
                }
            }
        }
    }

    auto& operator[](int index) {
        return historyMoves[index];
    }
};

inline History history;

class PVLine {
private:
    int length[MAX_DEPTH] = {};
    Move table[MAX_DEPTH][MAX_DEPTH] = {};
public:
    void SetLength(int ply) {
        length[ply] = ply;
    }

    void SetMove(int ply, Move move) {
        table[ply][ply] = move;

        for (int i = ply + 1; i < length[ply + 1]; i++) {
            table[ply][i] = table[ply+1][i];
        }

        length[ply] = length[ply + 1];
    }
    
    void Print(int n) {
        for (int i = 0; i < length[n]; i++) {
            table[n][i].PrintMove();
            std::cout << ' ';
        } 
    }

    void Clear() {
        length[MAX_DEPTH - 1] = {};
        table[MAX_DEPTH - 1][MAX_DEPTH - 1] = {};
    }
};

template <bool isPV>
SearchResults PVS(Board board, int depth, int alpha, int beta, int ply);

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
}
