#pragma once
#include "board.h"
#include "uci.h"
#include <vector>
#include "move.h"
#include <algorithm>
#include "stopwatch.h"
#include "tunables.h"
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
constexpr int MAX_DEPTH = 128;
constexpr int MAX_PLY = 512;
constexpr int MAX_HISTORY = 16384;

constexpr int CORRHIST_WEIGHT_SCALE = 256;
constexpr int CORRHIST_GRAIN = 256;
constexpr int CORRHIST_LIMIT = 1024;
constexpr int CORRHIST_SIZE = 16384;
constexpr int CORRHIST_MAX = 16384;

constexpr int32_t ScoreNone = -255000;
constexpr int inf = 100000;

inline int lmrTable[MAX_DEPTH+1][MAX_MOVES];

#ifdef TUNING
inline std::array<int, 6> SEEPieceValues = {
    seePawnValue, seeKnightValue, seeBishopValue, seeRookValue, seeQueenValue, 0
};
#else
constexpr std::array<int, 6> SEEPieceValues = {
    seePawnValue, seeKnightValue, seeBishopValue, seeRookValue, seeQueenValue, 0
};
#endif

void InitLMRTable();
class SearchContext;

class CorrHist {
private:
    // indexed by [stm][key]
    MultiArray<int, 2, CORRHIST_SIZE> pawnHist;
    MultiArray<int, 2, CORRHIST_SIZE> nonPawnHist;
public:
    void Update(Board& board, int depth, int diff, int* entry) {
        const int scaledDiff = diff * CORRHIST_GRAIN;
        const int newWeight = std::min(depth * depth + 2 * depth + 1, 128);

        *entry = (*entry * (CORRHIST_WEIGHT_SCALE - newWeight) + scaledDiff * newWeight) / CORRHIST_WEIGHT_SCALE;
        *entry = std::clamp(*entry, -CORRHIST_MAX, CORRHIST_MAX);
    }

    void UpdateAll(Board& board, int depth, int diff) {
        Update(board, depth, diff, &pawnHist[board.sideToMove][board.pawnKey % CORRHIST_SIZE]);
        Update(board, depth, diff, &nonPawnHist[board.sideToMove][board.nonPawnKey % CORRHIST_SIZE]);
    }

    int GetAllHist(Board& board) {
        return pawnHist[board.sideToMove][board.pawnKey % CORRHIST_SIZE]
            +  nonPawnHist[board.sideToMove][board.nonPawnKey % CORRHIST_SIZE];
    }

    void Clear() {
        std::fill(&pawnHist[0][0], &pawnHist[0][0] + sizeof(pawnHist) / sizeof(int), 0);
        std::fill(&nonPawnHist[0][0], &nonPawnHist[0][0] + sizeof(nonPawnHist) / sizeof(int), 0);
    }
};

class ContHistory {
private:
    // indexed by [other color][prev move piece][stm][prev move to][piece][to]
    MultiArray<int16_t, 2, 6, 64, 2, 6, 64> contHistMoves;
public:
    void Update(bool stm, bool otherColor, int prevType, int prevTo, int type, int to, int bonus) {
        int clampedBonus = std::clamp(bonus, -MAX_HISTORY, MAX_HISTORY);

        contHistMoves[otherColor][prevType][prevTo][stm][type][to] +=
            clampedBonus - contHistMoves[otherColor][prevType][prevTo][stm][type][to] * std::abs(clampedBonus) / MAX_HISTORY;
    }

    void Clear() {
        std::fill(&contHistMoves[0][0][0][0][0][0], &contHistMoves[0][0][0][0][0][0] + sizeof(contHistMoves) / sizeof(int16_t), 0);
    }

    int16_t GetOnePly(Board& board, Move& move, SearchContext* ctx, int ply);
    int16_t GetTwoPly(Board& board, Move& move, SearchContext* ctx, int ply);

    auto& operator[](int index) {
        return contHistMoves[index];
    }
};

class History {
private:
    // indexed by [stm][from][to][threatenedSource][threatenedTarget]
    MultiArray<int, 2, 64, 64, 2, 2> historyMoves;
public:
    void Update(bool stm, Move move, bool source, bool target, int bonus) {
        int clampedBonus = std::clamp(bonus, -MAX_HISTORY, MAX_HISTORY);
        historyMoves[stm][move.MoveFrom()][move.MoveTo()][source][target] +=
            clampedBonus - historyMoves[stm][move.MoveFrom()][move.MoveTo()][source][target] * std::abs(clampedBonus) / MAX_HISTORY;
    }

    void Clear() {
        std::fill(&historyMoves[0][0][0][0][0], &historyMoves[0][0][0][0][0] + sizeof(historyMoves) / sizeof(int), 0);
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
    bool side = -1;
};

class SearchContext {
public:
    U64 nodes = 0;
    U64 nodesToGo = 0;
    int timeToSearch = 0;

    std::array<U64, 4096> nodesTable{};

    int seldepth = 0;

    Move excluded = Move();

    bool doingNullMove = false;
    bool searchStopped = false;

    PVLine pvLine;
    History history;
    ContHistory conthist;
    CorrHist corrhist;

    std::array<std::array<int, MAX_PLY>, 2> killerMoves{};

    std::vector<U64> positionHistory;

    TTable TT;

    std::array<Stack, MAX_PLY> ss{};

    Stopwatch sw;

    SearchContext(){
        pvLine.Clear();
        history.Clear();
        conthist.Clear();
        corrhist.Clear();
        TT.Clear();
        sw.Restart();
        killerMoves = {};
        ss = {};
        positionHistory.resize(1000);
    }
};

bool SEE(Board& board, Move& move, int threshold);

template <bool isPV, searchMode mode>
SearchResults PVS(Board& board, int depth, int alpha, int beta, int ply, SearchContext* ctx, bool cutnode);

template <searchMode mode>
SearchResults SearchPosition(Board &board, SearchParams params, SearchContext* ctx);

bool IsDraw(Board &board, SearchContext* ctx);

void ListScores(Board &board, SearchContext* ctx);

int MoveEstimatedValue(Board& board, Move& move);
}
