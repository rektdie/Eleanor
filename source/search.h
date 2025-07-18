#pragma once
#include "board.h"
#include "uci.h"
#include <vector>
#include "move.h"
#include <algorithm>
#include "stopwatch.h"
#include "tt.h"

#include "../external/multi_array.h"

namespace SEARCH {

enum searchMode {
    normal,
    bench,
    datagen,
    nodesMode
};

constexpr int MATE_SCORE = 32767;
constexpr int MAX_DEPTH = 64;
constexpr int MAX_PLY = 128;
constexpr int MAX_HISTORY = 16384;

constexpr int32_t ScoreNone = -255000;
constexpr int inf = 100000;

inline int lmrTable[MAX_DEPTH+1][MAX_MOVES];

constexpr std::array<int, 6> SEEPieceValues = {
    100, 300, 350, 500, 900, 0
};

void InitLMRTable();

class ContHistory {
private:
    // indexed by [stm][prev move piece][prev move to][piece][to]
    MultiArray<int16_t, 2, 6, 64, 6, 64> contHistMoves;
public:
    void Update(bool stm, int prevType, int prevTo, int type, int to, int bonus) {
        int clampedBonus = std::clamp(bonus, -MAX_HISTORY, MAX_HISTORY);

        contHistMoves[stm][prevType][prevTo][type][to] +=
            clampedBonus - contHistMoves[stm][prevType][prevTo][type][to] * std::abs(clampedBonus) / MAX_HISTORY;
    }

    void Clear() {
        std::fill(&contHistMoves[0][0][0][0][0], &contHistMoves[0][0][0][0][0] + sizeof(contHistMoves) / sizeof(int16_t), 0);
    }

    auto& operator[](int index) {
        return contHistMoves[index];
    }
};

class History {
private:
    MultiArray<int, 2, 64, 64> historyMoves;
public:
    void Update(bool stm, Move move, int bonus) {
        int clampedBonus = std::clamp(bonus, -MAX_HISTORY, MAX_HISTORY);
        historyMoves[stm][move.MoveFrom()][move.MoveTo()] += 
            clampedBonus - historyMoves[stm][move.MoveFrom()][move.MoveTo()] * std::abs(clampedBonus) / MAX_HISTORY;
    }

    void Clear() {
        std::fill(&historyMoves[0][0][0], &historyMoves[0][0][0] + sizeof(historyMoves) / sizeof(int), 0);
    }

    auto& operator[](int index) {
        return historyMoves[index];
    }
};

class PVLine {
private:
    int length[MAX_PLY] = {};
    Move table[MAX_PLY][MAX_PLY] = {};
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

class Stack {
public:
    int32_t eval = ScoreNone;

    int pieceType = -1;
    int moveTo = -1;
};

class SearchContext {
public:
    U64 nodes = 0;
    U64 nodesToGo = 0;
    int timeToSearch = 0;

    int seldepth = 0;
    
    bool doingNullMove = false;
    bool searchStopped = false;

    PVLine pvLine;
    History history;
    ContHistory conthist;

    std::array<std::array<int, MAX_PLY>, 2> killerMoves{};

    std::vector<U64> positionHistory;

    TTable TT;

    std::array<Stack, MAX_PLY> ss{};

    Stopwatch sw;

    SearchContext(){
        pvLine.Clear();
        history.Clear();
        conthist.Clear();
        TT.Clear();
        sw.Restart();
        killerMoves = {};
        positionHistory.reserve(1000);
    }
};

bool SEE(Board& board, Move& move, int threshold);

template <bool isPV, searchMode mode>
SearchResults PVS(Board& board, int depth, int alpha, int beta, int ply, SearchContext* ctx, bool cutnode);

template <searchMode mode>
SearchResults SearchPosition(Board &board, SearchParams params, SearchContext* ctx);

bool IsDraw(Board &board, SearchContext* ctx);

constexpr int moveScoreTable[6][6] = {
    105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600
};

void ListScores(Board &board, SearchContext* ctx);

int MoveEstimatedValue(Board& board, Move& move);
}
