#pragma once

#include <algorithm>
#include <cmath>

#include "board.h"

struct WinRateParams {
    double a;
    double b;
};

// From SF
inline WinRateParams winRateParams(const Board& board) {
    int material = board.pieces[Pawn].PopCount() +
               3 * board.pieces[Knight].PopCount() +
               3 * board.pieces[Bishop].PopCount() +
               5 * board.pieces[Rook].PopCount() +
               9 * board.pieces[Queen].PopCount();

    // The fitted model only uses data for material counts in [17, 78], and is anchored at count 58.
    double m = std::clamp(material, 17, 78) / 58.0;

    // Return a = p_a(material) and b = p_b(material), see github.com/official-stockfish/WDL_model
    constexpr double as[] = {-977.81543545, 2519.99289616, -2279.71042020, 1197.60065700};
    constexpr double bs[] = {-224.61509659, 557.03359920, -440.33347428, 226.68436468};

    double a = (((as[0] * m + as[1]) * m + as[2]) * m) + as[3];
    double b = (((bs[0] * m + bs[1]) * m + bs[2]) * m) + bs[3];

    return {a, b};
}

// The win rate model is 1 / (1 + exp((a - eval) / b)), where a = p_a(material) and b = p_b(material).
// It fits the LTC fishtest statistics rather accurately.
inline int winRateModel(int v, const Board& board) {
    auto [a, b] = winRateParams(board);

    // Return the win rate in per mille units, rounded to the nearest integer.
    return int(0.5 + 1000 / (1 + std::exp((a - static_cast<double>(v)) / b)));
}

struct WDLTriplet {
    int wins;
    int draws;
    int losses;
};

inline WDLTriplet getWDL(int v, const Board& board) {
    int wins = std::clamp(winRateModel(v, board), 0, 1000);
    int losses = std::clamp(winRateModel(-v, board), 0, 1000);
    int draws = 1000 - wins - losses;

    if (draws < 0) {
        if (wins >= losses) {
            wins += draws;
        } else {
            losses += draws;
        }

        draws = 0;
    }

    return {wins, draws, losses};
}

inline int scaleEval(int eval, const Board& board) {
    auto [a, b] = winRateParams(board);

    return std::round(100 * eval / a);
}
