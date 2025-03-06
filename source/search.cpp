#include "search.h"
#include "movegen.h"
#include "evaluate.h"
#include <algorithm>
#include <chrono>
#include "tt.h"

int nodes = 0;
bool benchStarted = false;

static inline int ply = 0;
static inline auto timeStart = std::chrono::high_resolution_clock::now();
static inline int timeToSearch = 0;
static inline bool searchStopped = false;
static inline bool doingNullMove = false;

//                          [id][ply]
static inline int killerMoves[2][64];

static int ScoreMove(Board &board, Move &move) {
    const int attackerType = board.GetPieceType(move.MoveFrom());
    const int targetType = board.GetPieceType(move.MoveTo());

    if (attackerType == nullPieceType || targetType == nullPieceType) return 0;

    TTEntry *current = &TTable[board.hashKey % hashSize];
    if (current->bestMove == move) {
        return 50000;
    }

    if (attackerType == -1 || targetType == -1) std::cout << "asd\n";

    if (move.IsCapture()) {
        return moveScoreTable[attackerType][targetType] + 10000;
    } else {
        if (killerMoves[0][ply] == move) {
            return 9000;
        }

        if (killerMoves[1][ply] == move) {
            return 8000;
        }
    }

    return 0;
}

static void SortMoves(Board &board) {
    std::array<int, 218> scores;
    std::array<int, 218> indices;

    // Initialize scores and indices
    for (int i = 0; i < board.currentMoveIndex; i++) {
        scores[i] = ScoreMove(board, board.moveList[i]);
        indices[i] = i;
    }

    // Sort indices based on scores
    std::stable_sort(indices.begin(), indices.begin() + board.currentMoveIndex,
              [&scores](int a, int b) {
                  return scores[a] > scores[b];
              });

    // Create a temporary move list and reorder the moveList based on the sorted indices
    std::array<Move, 218> sortedMoves;
    for (int i = 0; i < board.currentMoveIndex; i++) {
        sortedMoves[i] = board.moveList[indices[i]];
    }

    // Assign the sorted moves back to the board's move list
    for (int i = 0; i < board.currentMoveIndex; i++) {
        board.moveList[i] = sortedMoves[i];
    }
}

void ListScores(Board &board) {
    SortMoves(board);

    for (int i = 0; i < board.currentMoveIndex; i++) {
        Move currentMove = board.moveList[i];

        std::cout << squareCoords[currentMove.MoveFrom()] << squareCoords[currentMove.MoveTo()];
        std::cout << ": " << ScoreMove(board, currentMove) << '\n';
    }
}

static SearchResults Quiescence(Board board, int alpha, int beta) {
    if (!benchStarted) {
        auto currTime = std::chrono::high_resolution_clock::now();
        int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currTime - timeStart).count();
        if (elapsed >= timeToSearch) {
            StopSearch();
            return 0;
        }
    }

    nodes++;

    int staticScore = Evaluate(board);

    if (staticScore >= beta) {
        return beta;
    }

    if (staticScore > alpha) {
        alpha = staticScore;
    }

    GenerateMoves(board, board.sideToMove);

    SortMoves(board);

    SearchResults results;

    for (int i = 0; i < board.currentMoveIndex; i++) {
        if (board.moveList[i].IsCapture()) {
            Board copy = board;
            copy.MakeMove(board.moveList[i]);
            int score = -Quiescence(copy, -beta, -alpha).score;

            if (score >= beta) {
                return beta;
            }

            if (score > alpha) {
                alpha = score;
                results.bestMove = board.moveList[i];
            }
        }
    }

    results.score = alpha;
    if (searchStopped) return 0;
    return results;
}

SearchResults PVS(Board board, int depth, int alpha, int beta) {
    if (!benchStarted) {
        auto currTime = std::chrono::high_resolution_clock::now();
        int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currTime - timeStart).count();

        if (elapsed >= timeToSearch) {
            StopSearch();
            return 0;
        }
    }

    // if NOT PV node then we try to hit the TTable
    if (beta - alpha == 1) {
        SearchResults entry = ReadEntry(board.hashKey, depth, alpha, beta);
        if (entry.score != invalidEntry) {
            return entry;
        }
    }

    if (depth <= 0) return Quiescence(board, alpha, beta);

    if (board.InCheck(board.sideToMove)) {
        depth++;
    } else {
        //Null Move Pruning
        if (!doingNullMove) {
            if (ply && depth >= 3 && !board.InPossibleZug(board.sideToMove)) {
                Board copy = board;
                copy.MakeMove(Move());

                doingNullMove = true;
                ply++;
                int score = -PVS(copy, depth - 3, -beta, -beta + 1).score;
                ply--;
                doingNullMove = false;

                if (searchStopped) return 0;
                if (score >= beta) return score; 
            }
        }
    }

    int score = -inf;
    int nodeType = AllNode;

    nodes++;

    GenerateMoves(board, board.sideToMove);
    SortMoves(board);

    if (board.currentMoveIndex == 0) {
        if (board.InCheck(board.sideToMove)) { // checkmate
            return -99000 + ply;
        } else { // stalemate
            return 0;
        }
    }

    SearchResults results;

    // For all moves
    for (int i = 0; i < board.currentMoveIndex; i++) {
        Board copy = board;
        copy.MakeMove(copy.moveList[i]);

        // First move (suspected PV node)
        if (!i) {
            // Full search
            ply++;
            score = -PVS(copy, depth - 1, -beta, -alpha).score;
            ply--;
        } else {
            // Quick search
            ply++;
            score = -PVS(copy, depth - 1, -alpha-1, -alpha).score;
            ply--;

            if (score > alpha && beta - alpha > 1) {
                // We have to do full search
                ply++;
                score = -PVS(copy, depth - 1, -beta, -alpha).score;
                ply--;
            }
        }

        // Fail high (beta cutoff)
        if (score >= beta) {
            killerMoves[1][ply] = killerMoves[0][ply];
            killerMoves[0][ply] = board.moveList[i];

            WriteEntry(board.hashKey, depth, beta, CutNode, Move());
            return beta;
        }

        if (score > alpha) {
            nodeType = PV;
            alpha = score;
            results.bestMove = board.moveList[i];
        }
    }


    results.score = alpha;
    WriteEntry(board.hashKey, depth, results.score, nodeType, results.bestMove);
    if (searchStopped) return 0;
    return results;
}

// Iterative deepening
static SearchResults ID(Board &board, SearchParams params) {
    timeStart = std::chrono::high_resolution_clock::now();

    int fullTime = board.sideToMove ? params.btime : params.wtime;
    int inc = board.sideToMove ? params.binc : params.winc;

    SearchResults safeResults;
    safeResults.score = -inf;

    int alpha = -inf;
    int beta = inf;

    for (int depth = 1; depth <= 99; depth++) {
        timeToSearch = (fullTime / 20) + (inc / 2);

        SearchResults currentResults = PVS(board, depth, alpha, beta);

        // If we fell outside the window, try again with full width
        if ((currentResults.score <= alpha)
            || (currentResults.score >= beta)) {
            alpha = -inf;
            beta = inf;
            depth--;

            if (searchStopped) break;
            continue;
        }

        alpha = currentResults.score - aspWinConst;
        beta = currentResults.score + aspWinConst;

        if (currentResults.bestMove) {
            safeResults = currentResults;
        }

        if (searchStopped) break;
    }

    return safeResults;
}

void SearchPosition(Board &board, SearchParams params) {
    searchStopped = false;
    nodes = 0;

    SearchResults results = ID(board, params);
    auto currTime = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration_cast<
        std::chrono::duration<double>>(currTime - timeStart).count();

    std::cout << "info " << nodes << " nodes " << int(nodes/elapsed) << " nps\n";
    std::cout << "bestmove " << squareCoords[results.bestMove.MoveFrom()]
        << squareCoords[results.bestMove.MoveTo()];

    int flags = results.bestMove.GetFlags();
    if (flags == knightPromotion || flags == knightPromoCapture) {
        std::cout << 'n';
    } else if (flags == bishopPromotion || flags == bishopPromoCapture) {
        std::cout << 'b';
    } else if (flags == rookPromotion || flags == rookPromoCapture) {
        std::cout << 'r';
    } else if (flags == queenPromotion || flags == queenPromoCapture) {
        std::cout << 'q';
    }

    std::cout << '\n';
}

void StopSearch() {
    searchStopped = true;
}
