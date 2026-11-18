#include "perft.h"
#include "stopwatch.h"
#include "movegen.h"
#include "termcolor.hpp"
#include <iostream>
#include <iomanip>
#include <string>

struct PerftCounts {
    U64 nodes = 0;
    U64 captures = 0;
    U64 ep = 0;
    U64 castles = 0;
    U64 promotions = 0;
    U64 checks = 0;
    U64 discovery = 0;
    U64 doubles = 0;
    U64 mates = 0;

    PerftCounts& operator+=(const PerftCounts& other) {
        nodes += other.nodes;
        captures += other.captures;
        ep += other.ep;
        castles += other.castles;
        promotions += other.promotions;
        checks += other.checks;
        discovery += other.discovery;
        doubles += other.doubles;
        mates += other.mates;
        return *this;
    }
};

static bool IsDirectCheck(Board& pos, Move move) {
    int mover = !pos.sideToMove;
    int kingSq = (pos.pieces[King] & pos.colors[pos.sideToMove]).getLS1BIndex();
    if (kingSq < 0 || kingSq >= 64)
        return false;
    int to = move.MoveTo();
    int flags = move.GetFlags();
    if (flags == kingCastle || flags == queenCastle) {
        int rookFrom = mover ? (flags == kingCastle ? h8 : a8) : (flags == kingCastle ? h1 : a1);
        int rookTo = flags == kingCastle ? rookFrom - 2 : rookFrom + 3;
        Bitboard rookAtt = MOVEGEN::getRookAttack(rookTo, pos.occupied);
        return rookAtt.IsSet(kingSq);
    }
    int endPiece = pos.GetPieceType(to);
    if (endPiece == nullPieceType || endPiece == King)
        return false;
    Bitboard att = MOVEGEN::getPieceAttacks(to, endPiece, mover, pos.occupied);
    return att.IsSet(kingSq);
}

static void CountLeaf(Board& pos, Move move, PerftCounts& out) {
    out.nodes++;
    int flags = move.GetFlags();
    if (move.IsCapture())
        out.captures++;
    if (flags == epCapture)
        out.ep++;
    if (flags == kingCastle || flags == queenCastle)
        out.castles++;
    if (move.IsPromo())
        out.promotions++;
    if (pos.InCheck()) {
        out.checks++;
        if (pos.checkers.PopCount() == 2)
            out.doubles++;
        if (!IsDirectCheck(pos, move))
            out.discovery++;
        MOVEGEN::GenerateMoves<All>(pos, true);
        if (!pos.currentMoveIndex)
            out.mates++;
    }
}

static void PerftRec(Board& board, int depth, PerftCounts& out) {
    MOVEGEN::GenerateMoves<All>(board, true);
    for (int i = 0; i < board.currentMoveIndex; i++) {
        Move move = board.moveList[i];
        Board copy = board;
        copy.MakeMove(move);
        if (depth == 1)
            CountLeaf(copy, move, out);
        else
            PerftRec(copy, depth - 1, out);
    }
}

static U64 PerftBulk(Board& board, int depth) {
    MOVEGEN::GenerateMoves<All>(board, true);
    if (depth == 1)
        return board.currentMoveIndex;

    U64 nodes = 0;
    for (int i = 0; i < board.currentMoveIndex; i++) {
        Board copy = board;
        copy.MakeMove(board.moveList[i]);
        nodes += PerftBulk(copy, depth - 1);
    }
    return nodes;
}

static void PrintPerftSummary(int depth, U64 nodes, double elapsed) {
    std::cout << std::endl << "Depth: " << depth << std::endl;
    std::cout << "Total nodes: " << nodes << std::endl;
    std::cout << "Time took: " << elapsed << "s (";
    std::cout << U64(double(nodes) / elapsed) << " nodes/sec)" << std::endl;
}

static void PerftDivide(Board &board, int depth) {
    Stopwatch sw;

    MOVEGEN::GenerateMoves<All>(board, true);

    U64 total = 0;
    for (int i = 0; i < board.currentMoveIndex; i++) {
        Move move = board.moveList[i];
        U64 nodes = 1;
        if (depth > 1) {
            Board copy = board;
            copy.MakeMove(move);
            nodes = PerftBulk(copy, depth - 1);
        }
        total += nodes;

        std::cout << termcolor::bold << termcolor::bright_cyan << std::setw(6) << std::left << (move.GetMoveString() + ":");
        std::cout << termcolor::bright_white << std::setw(11) << std::right << nodes << termcolor::reset << std::endl;
    }

    PrintPerftSummary(depth, total, sw.GetElapsedSec());
}

static void PrintPerftHeader() {
    std::cout << termcolor::bold << termcolor::color<244>;
    std::cout << std::setw(6) << std::left << "Move";
    std::cout << std::setw(11) << std::right << "Nodes";
    std::cout << std::setw(8) << std::right << "Capt";
    std::cout << std::setw(7) << std::right << "E.P.";
    std::cout << std::setw(7) << std::right << "Cast";
    std::cout << std::setw(7) << std::right << "Promo";
    std::cout << std::setw(8) << std::right << "Check";
    std::cout << std::setw(7) << std::right << "Disc";
    std::cout << std::setw(5) << std::right << "Dbl";
    std::cout << std::setw(7) << std::right << "Mate";
    std::cout << termcolor::reset << std::endl;

    std::cout << termcolor::color<238>;
    for (int i = 0; i < 73; i++)
        std::cout << '-';
    std::cout << termcolor::reset << std::endl;
}

static void PrintPerftBase(int index) {
    if (index % 2 == 0)
        std::cout << termcolor::color<247>;
    else
        std::cout << termcolor::color<251>;
}

static void PrintPerftLine(int index, const std::string& moveStr, const PerftCounts& c) {
    PrintPerftBase(index);

    std::cout << termcolor::bold << termcolor::bright_cyan;
    std::cout << std::setw(6) << std::left << (moveStr + ":");
    std::cout << termcolor::reset;
    PrintPerftBase(index);

    std::cout << termcolor::bold << termcolor::bright_white;
    std::cout << std::setw(11) << std::right << c.nodes;
    std::cout << termcolor::reset;
    PrintPerftBase(index);

    if (c.captures > 0)
        std::cout << termcolor::bold << termcolor::yellow;
    else
        std::cout << termcolor::color<244>;
    std::cout << std::setw(8) << std::right << c.captures;
    std::cout << termcolor::reset;
    PrintPerftBase(index);

    if (c.ep > 0)
        std::cout << termcolor::bold << termcolor::bright_yellow;
    else
        std::cout << termcolor::color<244>;
    std::cout << std::setw(7) << std::right << c.ep;
    std::cout << termcolor::reset;
    PrintPerftBase(index);

    if (c.castles > 0)
        std::cout << termcolor::bold << termcolor::cyan;
    else
        std::cout << termcolor::color<244>;
    std::cout << std::setw(7) << std::right << c.castles;
    std::cout << termcolor::reset;
    PrintPerftBase(index);

    if (c.promotions > 0)
        std::cout << termcolor::bold << termcolor::magenta;
    else
        std::cout << termcolor::color<244>;
    std::cout << std::setw(7) << std::right << c.promotions;
    std::cout << termcolor::reset;
    PrintPerftBase(index);

    if (c.checks > 0)
        std::cout << termcolor::bold << termcolor::green;
    else
        std::cout << termcolor::color<244>;
    std::cout << std::setw(8) << std::right << c.checks;
    std::cout << termcolor::reset;
    PrintPerftBase(index);

    if (c.discovery > 0)
        std::cout << termcolor::bold << termcolor::bright_magenta;
    else
        std::cout << termcolor::color<244>;
    std::cout << std::setw(7) << std::right << c.discovery;
    std::cout << termcolor::reset;
    PrintPerftBase(index);

    if (c.doubles > 0)
        std::cout << termcolor::bold << termcolor::bright_red;
    else
        std::cout << termcolor::color<244>;
    std::cout << std::setw(5) << std::right << c.doubles;
    std::cout << termcolor::reset;
    PrintPerftBase(index);

    if (c.mates > 0)
        std::cout << termcolor::bold << termcolor::red;
    else
        std::cout << termcolor::color<244>;
    std::cout << std::setw(7) << std::right << c.mates;
    std::cout << termcolor::reset << std::endl;
}

void Perft(Board &board, int depth, bool stats) {
    Stopwatch sw;

    if (depth > 0 && !stats) {
        PerftDivide(board, depth);
        return;
    }

    if (depth <= 0) {
        std::cout << std::endl << "Depth: " << depth << std::endl;
        std::cout << "Total nodes: 1" << std::endl;
        std::cout << "Time took: " << sw.GetElapsedSec() << "s (";
        std::cout << int(1 / sw.GetElapsedSec()) << " nodes/sec)" << std::endl;
        return;
    }

    MOVEGEN::GenerateMoves<All>(board, true);

    PerftCounts total;
    int moveCount = 0;

    PrintPerftHeader();

    for (int i = 0; i < board.currentMoveIndex; i++) {
        Move move = board.moveList[i];
        Board copy = board;
        copy.MakeMove(move);
        PerftCounts local;
        if (depth == 1)
            CountLeaf(copy, move, local);
        else
            PerftRec(copy, depth - 1, local);
        total += local;
        PrintPerftLine(moveCount, move.GetMoveString(), local);
        moveCount++;
    }

    double elapsed = sw.GetElapsedSec();

    std::cout << termcolor::color<238>;
    for (int i = 0; i < 73; i++)
        std::cout << '-';
    std::cout << termcolor::reset << std::endl;

    PrintPerftLine(moveCount, "Total", total);

    std::cout << std::endl;
    std::cout << termcolor::bold << termcolor::bright_white;
    std::cout << "Depth: " << depth;
    std::cout << "   Moves: " << moveCount;
    std::cout << "   Nodes: " << total.nodes;
    U64 nps = elapsed > 0 ? U64(double(total.nodes) / elapsed) : total.nodes;
    std::cout << "   Time: ";
    if (elapsed >= 1.0)
        std::cout << elapsed << "s";
    else
        std::cout << int(elapsed * 1000) << "ms";
    std::cout << "   Avg: " << nps << "/s";
    std::cout << termcolor::reset << std::endl;

    PrintPerftSummary(depth, total.nodes, elapsed);
}
