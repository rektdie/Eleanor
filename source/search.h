#pragma once
#include "board.h"
#include "uci.h"
#include <vector>
#include "move.h"
#include <algorithm>

namespace SEARCH {

constexpr int maxDepth = 64;
constexpr int maxHistory = 16384;

extern U64 nodes;
extern bool benchStarted;

constexpr int inf = 100000;

inline bool searchStopped = false;

//                   [id][ply]
inline int killerMoves[2][64];

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
        int clampedBonus = std::clamp(bonus, -maxHistory, maxHistory);
        historyMoves[stm][move.MoveFrom()][move.MoveTo()] += 
            clampedBonus - historyMoves[stm][move.MoveFrom()][move.MoveTo()] * std::abs(clampedBonus) / maxHistory;
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
    int length[maxDepth] = {};
    Move table[maxDepth][maxDepth] = {};
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
        length[maxDepth - 1] = {};
        table[maxDepth - 1][maxDepth - 1] = {};
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
