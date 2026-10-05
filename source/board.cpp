#include <bit>
#include "board.h"
#include "accumulator.h"
#include "movegen.h"
#include "nnue.h"
#include "tt.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <vector>
#include <ranges>
#include <string_view>
#include <cassert>
#include "types.h"
#include "utils.h"
#include "wdl.h"
#include "search.h"
#include "termcolor.hpp"

void Board::Reset() {
    castlingRights = 0;
	enPassantTarget = noEPTarget;

	halfMoves = 0;
	fullMoves = 1;

	sideToMove = White;
	occupied = 0ULL;

    checkers = 0ULL;

    mailbox = std::array<int, 64>();
    mailbox.fill(nullPieceType);

    pinned = std::array<Bitboard, 2>();

	pieces = std::array<Bitboard, 6>();
	colors = std::array<Bitboard, 2>();

	accUpdate.setRefresh();
	mirroredWhite = false;
	mirroredBlack = false;

	pieceThreats = std::array<Bitboard, 6>();
	colorThreats = std::array<Bitboard, 2>();

    moveList = std::array<Move, MAX_MOVES>();

	positionIndex = 0;

    currentMoveIndex = 0;

    hashKey = 0ULL;
    pawnKey = 0ULL;
    nonPawnKey = 0ULL;
    majorKey = 0ULL;

    checkZones = std::array<Bitboard, 4>();
}

void Board::SetByFen(std::string_view fen) {
	Reset();

	int currSquare = a8;

	std::vector<std::string> tokens = UTILS::split(fen, ' ');
	std::vector<std::string> pieceTokens = UTILS::split(tokens[0], '/');

	constexpr std::string_view pieceTypes = "pnbrqk";

	for (std::string_view rank : pieceTokens) {
		for (const char piece : rank) {
			if (std::isdigit(piece)) {
				currSquare += piece - '0';
				continue;
			}

			bool side = std::islower(piece);
			int pieceType = pieceTypes.find(std::tolower(piece));

            SetPiece(pieceType, currSquare, side);

			currSquare++;
		}
		currSquare -= 16;
	}

	sideToMove = tokens[1] == "b";

    for (const char piece : tokens[2]) {
        if (piece == 'K') castlingRights |= whiteKingRight;
        else if (piece == 'Q') castlingRights |= whiteQueenRight;
        else if (piece == 'k') castlingRights |= blackKingRight;
        else if (piece == 'q') castlingRights |= blackQueenRight;
    }

    if (tokens[3] != "-") enPassantTarget = UTILS::parseSquare(tokens[3]);

	if (tokens.size() > 4) {
		halfMoves = std::stoi(tokens[4]);
		fullMoves = std::stoi(tokens[5]);
	}

    pawnKey = 0ULL;
    nonPawnKey = 0ULL;
    majorKey = 0ULL;

    occupied = colors[White] | colors[Black];
    hashKey = UTILS::GetHashKey(*this);
    checkers = CalcCheckers();
    pinned = {CalcPinned(White), CalcPinned(Black)};
    CalcCheckers();
	MOVEGEN::GenThreatMaps(*this);
	MOVEGEN::GenerateMoves<All>(*this, true);

    int whiteKingFile = (pieces[King] & colors[White]).getLS1BIndex() % 8;
    int blackKingFile = (pieces[King] & colors[Black]).getLS1BIndex() % 8;

    if (whiteKingFile > 3) {
        mirroredWhite = true;
    }
    if (blackKingFile > 3) {
        mirroredBlack = true;
    }

    accUpdate.setRefresh();
}

std::string Board::GetFen() {
    std::ostringstream fen;

    for (int rank = 7; rank >= 0; --rank) {
        int emptyCount = 0;

        for (int file = 0; file < 8; ++file) {
            int square = rank * 8 + file;
            const uint64_t squareBit = 1ULL << square;

            if (!(occupied & squareBit)) {
                ++emptyCount;
                continue;
            }

            if (emptyCount > 0) {
                fen << emptyCount;
                emptyCount = 0;
            }

            static constexpr std::array<char, 6> pieceChars = {'p', 'n', 'b', 'r', 'q', 'k'};
            char pieceChar = '?';

            for (size_t pieceType = 0; pieceType < pieceChars.size(); ++pieceType) {
                if (pieces[pieceType] & squareBit) {
                    pieceChar = pieceChars[pieceType];
                    break;
                }
            }

            fen << (colors[White] & squareBit ? static_cast<char>(std::toupper(pieceChar)) : pieceChar);
        }

        if (emptyCount > 0) {
            fen << emptyCount;
        }

        if (rank > 0) {
            fen << '/';
        }
    }

    fen << ' ' << (sideToMove ? 'b' : 'w');

    fen << ' ';
    bool hasCastlingRights = false;

    const std::array<std::pair<uint8_t, char>, 4> castlingOptions = {
        {{whiteKingRight, 'K'}, {whiteQueenRight, 'Q'}, {blackKingRight, 'k'}, {blackQueenRight, 'q'}}
    };

    for (const auto& [right, symbol] : castlingOptions) {
        if (castlingRights & right) {
            fen << symbol;
            hasCastlingRights = true;
        }
    }

    if (!hasCastlingRights) {
        fen << '-';
    }

    fen << ' ';
    if (enPassantTarget == noEPTarget) {
        fen << '-';
    } else {
        const int file = enPassantTarget % 8;
        const int rank = enPassantTarget / 8;
        fen << static_cast<char>('a' + file) << (rank + 1);
    }

    fen << ' ' << halfMoves;

    fen << ' ' << fullMoves;

    return fen.str();
}

void Board::PrintBoard() {

	for (int rank = 7; rank >= 0; rank--) {

		std::cout << "+---+---+---+---+---+---+---+---+" << std::endl;
		std::cout << "| ";

		for (int file = 0; file < 8; file++) {
			int square = rank * 8 + file;
			bool pieceSet = false;

			for (int i = Pawn; i <= King; i++) {
				if ((colors[White] & pieces[i]).IsSet(square)) {
					std::cout << PIECE_LETTERS[i * 2 + White] << " | ";
					pieceSet = true;
				} else if ((colors[Black] & pieces[i]).IsSet(square)) {
					std::cout << PIECE_LETTERS[i * 2 + Black] << " | ";
					pieceSet = true;
				}
			}

			if (!pieceSet) {
				std::cout << "  | ";
			}
		}
		std::cout << ' ' << rank + 1 << std::endl;
	}

	std::cout << "+---+---+---+---+---+---+---+---+" << std::endl;
	std::cout << "  a   b   c   d   e   f   g   h" << std::endl << std::endl;
	std::cout << "      Side to move: ";
	if (!sideToMove) {
		std::cout << "White" << std::endl;
	} else {
		std::cout << "Black" << std::endl;
	}
	if (enPassantTarget != noEPTarget) {
		std::cout << "      En Passant square: " << squareCoords[enPassantTarget] << std::endl;
	} else {
		std::cout << "      En Passant square: None" << std::endl;
	}

	std::cout << "      Castling rights: ";
	if (castlingRights & whiteKingRight) std::cout << "K"; else std::cout << "-";
	if (castlingRights & whiteQueenRight) std::cout << "Q"; else std::cout << "-";
	if (castlingRights & blackKingRight) std::cout << "k"; else std::cout << "-";
	if (castlingRights & blackQueenRight) std::cout << "q"; else std::cout << "-";
	std::cout << std::endl << std::endl;

    std::cout << "      Hashkey: 0x" << std::hex << hashKey << std::dec << std::endl;
	std::cout << "      Fen: " << GetFen() << std::endl;
}

void Board::PrintNNUE(bool full) {
    ACC::AccumulatorPair acc;
    RefreshAccumulator(acc);

    const int pieceCount = occupied.PopCount();
    const size_t divisor = 32 / NNUE::OUTPUT_BUCKETS;
    const size_t outBucket = (pieceCount - 2) / divisor;

    const int wKingSq = (pieces[King] & colors[White]).getLS1BIndex();
    const int bKingSq = (pieces[King] & colors[Black]).getLS1BIndex();
    const ACC::BucketPair buckets = GetBuckets();

    const bool stmIsWhite = (sideToMove == White);
    const ACC::Accumulator& stmAcc = stmIsWhite ? acc.white : acc.black;
    const ACC::Accumulator& nstmAcc = stmIsWhite ? acc.black : acc.white;

    auto bar = [](int value, int maxValue, int width) {
        std::string s;
        int filled = maxValue > 0 ? (value * width + maxValue / 2) / maxValue : 0;
        for (int i = 0; i < width; i++)
            s += (i < filled) ? '#' : '.';
        return s;
    };

    auto section = [](const char* name) {
        std::cout << termcolor::bold << termcolor::color<244>;
        std::cout << "-- " << name << " --";
        std::cout << termcolor::reset << std::endl;
    };

    auto pieceLetter = [&](int sq) {
        int pt = mailbox[sq];
        if (pt == nullPieceType)
            return ' ';
        bool white = colors[White].IsSet(sq);
        return PIECE_LETTERS[pt * 2 + (white ? White : Black)];
    };

    auto boardRow = [&](int rank) {
        std::ostringstream row;
        row << termcolor::color<244> << rank + 1 << termcolor::reset << "  ";
        for (int file = 0; file < 8; file++) {
            int sq = rank * 8 + file;
            int pt = mailbox[sq];
            bool isKingSq = (sq == wKingSq) || (sq == bKingSq);
            if (pt == nullPieceType) {
                row << termcolor::color<240> << ". " << termcolor::reset;
            } else {
                bool white = colors[White].IsSet(sq);
                if (isKingSq)
                    row << termcolor::bold << termcolor::reverse;
                if (white)
                    row << termcolor::bright_white;
                else
                    row << termcolor::yellow;
                row << pieceLetter(sq) << ' ';
                row << termcolor::reset;
            }
        }
        return row.str();
    };

    auto bucketMapRow = [&](bool forWhite, int rank) {
        std::ostringstream row;
        int kingSq = forWhite ? wKingSq : bKingSq;
        for (int file = 0; file < 8; file++) {
            int sq = rank * 8 + file;
            int b = NNUE::kingBuckets[forWhite ? White : Black][sq];
            if (sq == kingSq)
                row << termcolor::bold << termcolor::reverse << termcolor::bright_cyan;
            else
                row << termcolor::color<244>;
            row << b << ' ';
            row << termcolor::reset;
        }
        return row.str();
    };

    auto printBoardAndMaps = [&]() {
        std::cout << "    a b c d e f g h    White regions      Black regions" << std::endl;
        for (int rank = 7; rank >= 0; rank--) {
            std::cout << boardRow(rank) << "  " << bucketMapRow(true, rank);
            std::cout << "  " << bucketMapRow(false, rank) << std::endl;
        }
        std::cout << "    a b c d e f g h" << termcolor::color<244> << "    Kings highlighted" << termcolor::reset << std::endl;
    };

    auto printGauge = [&](double pawns, const std::string& valueStr, bool positive, bool isMate) {
        const int width = 41;
        const int center = width / 2;
        double cl = std::clamp(pawns, -5.0, 5.0);
        int mark = center + int(std::round(cl / 5.0 * center));
        mark = std::clamp(mark, 0, width - 1);
        std::cout << "  ";
        for (int i = 0; i < width; i++) {
            if (i == mark) {
                std::cout << termcolor::bold << termcolor::bright_white << 'O';
            } else if (i == center) {
                std::cout << termcolor::color<244> << '+';
            } else if (i < center) {
                std::cout << termcolor::red << '-';
            } else {
                std::cout << termcolor::green << '-';
            }
            std::cout << termcolor::reset;
        }
        std::cout << "  ";
        if (isMate) {
            if (positive)
                std::cout << termcolor::bold << termcolor::bright_green;
            else
                std::cout << termcolor::bold << termcolor::bright_red;
        } else if (positive) {
            std::cout << termcolor::bold << termcolor::green;
        } else {
            std::cout << termcolor::bold << termcolor::red;
        }
        std::cout << valueStr << termcolor::reset << std::endl;
        std::cout << "  " << termcolor::color<244> << "-5" << std::string(center - 2, ' ') << "0"
                  << std::string(center - 2, ' ') << "+5" << termcolor::reset << std::endl;
    };

    auto printWdlBar = [&](const WDLTriplet& t) {
        const int width = 32;
        int w = (t.wins * width + 500) / 1000;
        int l = (t.losses * width + 500) / 1000;
        int d = width - w - l;
        std::cout << "  ";
        std::cout << termcolor::bright_green;
        for (int i = 0; i < w; i++) std::cout << '#';
        std::cout << termcolor::color<244>;
        for (int i = 0; i < d; i++) std::cout << '#';
        std::cout << termcolor::bright_red;
        for (int i = 0; i < l; i++) std::cout << '#';
        std::cout << termcolor::reset << std::endl;
    };

    struct AccStats {
        int16_t mn = 0;
        int16_t mx = 0;
        double mean = 0;
        double absMean = 0;
        int zeros = 0;
        int saturated = 0;
    };

    auto accStats = [](const ACC::Accumulator& a) {
        AccStats s;
        s.mn = a[0];
        s.mx = a[0];
        int64_t sum = 0;
        int64_t absSum = 0;
        for (size_t i = 0; i < NNUE::HL_SIZE; i++) {
            s.mn = std::min(s.mn, a[i]);
            s.mx = std::max(s.mx, a[i]);
            sum += a[i];
            absSum += std::abs(a[i]);
            int32_t c = std::clamp<int32_t>(a[i], 0, NNUE::QA);
            if (c == 0)
                s.zeros++;
            if (c >= NNUE::QA)
                s.saturated++;
        }
        s.mean = double(sum) / double(NNUE::HL_SIZE);
        s.absMean = double(absSum) / double(NNUE::HL_SIZE);
        return s;
    };

    auto perspectivePart = [&](const ACC::Accumulator& a, size_t base, std::vector<std::tuple<int64_t,int,int16_t,int16_t>>* top) {
        int64_t part = 0;
        for (size_t i = 0; i < NNUE::HL_SIZE; i++) {
            int32_t c = std::clamp<int32_t>(a[i], 0, NNUE::QA);
            int16_t w = NNUE::net.output_weights[outBucket][base + i];
            int16_t prod = int16_t(c * w);
            int64_t contrib = int64_t(c) * prod;
            part += contrib;
            if (top && (c > 0))
                top->emplace_back(std::llabs(contrib), int(i), int16_t(c), w);
        }
        return part;
    };

    std::vector<std::tuple<int64_t,int,int16_t,int16_t>> stmTop;
    std::vector<std::tuple<int64_t,int,int16_t,int16_t>> nstmTop;
    stmTop.reserve(NNUE::HL_SIZE);
    nstmTop.reserve(NNUE::HL_SIZE);
    const int64_t stmPart = perspectivePart(stmAcc, 0, &stmTop);
    const int64_t nstmPart = perspectivePart(nstmAcc, NNUE::HL_SIZE, &nstmTop);
    const int64_t preBias = (stmPart + nstmPart) / NNUE::QA;
    const int16_t bias = NNUE::net.output_bias[outBucket];
    const int64_t biased = preBias + bias;
    const int64_t rawEval = (biased * NNUE::SCALE) / (NNUE::QA * NNUE::QB);

    const int nN = pieces[Knight].PopCount();
    const int nB = pieces[Bishop].PopCount();
    const int nR = pieces[Rook].PopCount();
    const int nQ = pieces[Queen].PopCount();
    const int materialScale = 2048 + 90 * nN + 90 * nB + 180 * nR + 360 * nQ;
    const int16_t finalEval = std::clamp<int64_t>(rawEval * materialScale / 4096,
        (-SEARCH::MATE_SCORE + SEARCH::MAX_DEPTH), (SEARCH::MATE_SCORE - SEARCH::MAX_DEPTH));

    const bool isMateScore = std::abs(finalEval) + SEARCH::MAX_DEPTH >= SEARCH::MATE_SCORE;
    const int normalized = isMateScore ? finalEval : scaleEval(finalEval, *this);
    const WDLTriplet wdl = getWDL(finalEval, *this);

    std::cout << termcolor::bold << termcolor::bright_white;
    std::cout << "NNUE evaluation detail";
    std::cout << termcolor::reset << std::endl;
    std::cout << termcolor::color<244> << "FEN: " << termcolor::reset << GetFen() << std::endl;
    std::cout << termcolor::color<244> << "Side to move: " << termcolor::reset;
    if (!sideToMove) {
        std::cout << termcolor::bold << termcolor::bright_white << "White";
    } else {
        std::cout << termcolor::bold << termcolor::color<208> << "Black";
    }
    std::cout << termcolor::reset << "   " << termcolor::color<244> << "Pieces: " << termcolor::reset << pieceCount << std::endl;
    if (full)
        std::cout << std::endl;

    section("Position");
    printBoardAndMaps();
    if (full)
        std::cout << std::endl;

    section("Architecture");
    std::cout << "  HL " << termcolor::bold << termcolor::bright_white << NNUE::HL_SIZE << termcolor::reset;
    std::cout << "   Input " << termcolor::bold << termcolor::bright_white << NNUE::INPUT_SIZE << termcolor::reset;
    std::cout << "   In-buckets " << termcolor::bold << termcolor::bright_white << NNUE::INPUT_BUCKETS << termcolor::reset;
    std::cout << "   Out-buckets " << termcolor::bold << termcolor::bright_white << NNUE::OUTPUT_BUCKETS << termcolor::reset << std::endl;
    std::cout << "  QA " << termcolor::bold << termcolor::bright_white << NNUE::QA << termcolor::reset;
    std::cout << "   QB " << termcolor::bold << termcolor::bright_white << NNUE::QB << termcolor::reset;
    std::cout << "   Scale " << termcolor::bold << termcolor::bright_white << NNUE::SCALE << termcolor::reset << std::endl;
    if (full)
        std::cout << std::endl;

    section("Buckets");
    std::cout << "  Out ";
    for (size_t b = 0; b < NNUE::OUTPUT_BUCKETS; b++) {
        if (b == outBucket)
            std::cout << termcolor::bold << termcolor::bright_cyan << '[' << b << ']' << termcolor::reset;
        else
            std::cout << termcolor::color<244> << ' ' << b << ' ' << termcolor::reset;
    }
    std::cout << termcolor::color<244> << "  ((" << pieceCount << " - 2) / " << divisor << ")" << termcolor::reset << std::endl;
    std::cout << "  Kings " << termcolor::bold << termcolor::bright_white << squareCoords[wKingSq] << "->" << buckets.white << termcolor::reset;
    std::cout << (mirroredWhite ? "(m)" : "") << " ";
    std::cout << termcolor::bold << termcolor::bright_white << squareCoords[bKingSq] << "->" << buckets.black << termcolor::reset;
    std::cout << (mirroredBlack ? "(m)" : "") << "   Feats ";
    std::cout << termcolor::bold << termcolor::bright_white << pieceCount << termcolor::reset << std::endl;
    if (full) {
        std::cout << "  Material ";
        std::cout << "N " << termcolor::bold << termcolor::bright_white << nN << termcolor::reset;
        std::cout << " B " << termcolor::bold << termcolor::bright_white << nB << termcolor::reset;
        std::cout << " R " << termcolor::bold << termcolor::bright_white << nR << termcolor::reset;
        std::cout << " Q " << termcolor::bold << termcolor::bright_white << nQ << termcolor::reset;
        std::cout << "  scale = 2048+90N+90B+180R+360Q = ";
        std::cout << termcolor::bold << termcolor::bright_white << materialScale << termcolor::reset;
        std::cout << termcolor::color<244> << " (/4096)" << termcolor::reset << std::endl;
    } else {
        std::cout << "  Material scale ";
        std::cout << termcolor::bold << termcolor::bright_white << materialScale << termcolor::reset;
        std::cout << termcolor::color<244> << " (/4096)" << termcolor::reset << std::endl;
    }
    if (full)
        std::cout << std::endl;

    section("Accumulators");
    for (int p = 0; p < 2; p++) {
        const bool isWhite = (p == 0);
        const ACC::Accumulator& a = isWhite ? acc.white : acc.black;
        const AccStats s = accStats(a);
        const int active = int(NNUE::HL_SIZE) - s.zeros;
        std::cout << "  " << (isWhite ? "White" : "Black");
        if ((stmIsWhite && isWhite) || (!stmIsWhite && !isWhite))
            std::cout << termcolor::bold << termcolor::bright_cyan << " (STM)" << termcolor::reset;
        else
            std::cout << termcolor::color<244> << " (NSTM)" << termcolor::reset;
        std::cout << "  min " << termcolor::bold << termcolor::bright_white << s.mn << termcolor::reset;
        std::cout << "  max " << termcolor::bold << termcolor::bright_white << s.mx << termcolor::reset;
        std::cout << "  mean ";
        std::cout << termcolor::bright_white << std::fixed << std::setprecision(1) << s.mean << termcolor::reset;
        if (full) {
            std::cout << "  |mean| ";
            std::cout << termcolor::bright_white << std::fixed << std::setprecision(1) << s.absMean << termcolor::reset;
            std::cout << std::endl;
            std::cout << "    active " << termcolor::bold << termcolor::green << active << termcolor::reset;
            std::cout << termcolor::color<244> << " / " << NNUE::HL_SIZE << termcolor::reset;
            std::cout << "   zero " << termcolor::color<244> << s.zeros << termcolor::reset;
            std::cout << "   saturated(QA) ";
            if (s.saturated > 0)
                std::cout << termcolor::bold << termcolor::yellow << s.saturated << termcolor::reset;
            else
                std::cout << termcolor::color<244> << "0" << termcolor::reset;
            std::cout << std::endl;
        } else {
            std::cout << "  act " << termcolor::bold << termcolor::green << active << termcolor::reset;
            std::cout << termcolor::color<244> << "/" << NNUE::HL_SIZE << " sat ";
            if (s.saturated > 0)
                std::cout << termcolor::reset << termcolor::bold << termcolor::yellow << s.saturated << termcolor::reset;
            else
                std::cout << "0" << termcolor::reset;
            std::cout << std::endl;
        }

        if (full) {
            int bins[8] = {};
            for (size_t i = 0; i < NNUE::HL_SIZE; i++) {
                int32_t c = std::clamp<int32_t>(a[i], 0, NNUE::QA);
                int bin = std::min(7, (c * 8) / (NNUE::QA + 1));
                bins[bin]++;
            }
            int binMax = *std::max_element(bins, bins + 8);
            std::cout << "    act ";
            std::cout << termcolor::green << bar(bins[0], binMax, 8) << termcolor::reset;
            std::cout << ' ' << termcolor::color<244> << "0" << termcolor::reset;
            std::cout << ' ' << termcolor::green << bar(bins[7], binMax, 8) << termcolor::reset;
            std::cout << ' ' << termcolor::color<244> << "QA" << termcolor::reset;
            std::cout << termcolor::color<244> << "   peak bin " << (std::max_element(bins, bins + 8) - bins);
            std::cout << " (" << binMax << ")" << termcolor::reset << std::endl;
        }
    }
    if (full)
        std::cout << std::endl;

    section("Output layer");
    if (full) {
        std::cout << "  bias[" << outBucket << "] = ";
        std::cout << termcolor::bold << termcolor::bright_white << bias << termcolor::reset << std::endl;
        std::cout << "  STM" << termcolor::color<244> << (stmIsWhite ? " (White)" : " (Black)") << termcolor::reset;
        std::cout << " part = " << termcolor::bold << termcolor::bright_white << stmPart << termcolor::reset << std::endl;
        std::cout << "  NSTM" << termcolor::color<244> << (stmIsWhite ? " (Black)" : " (White)") << termcolor::reset;
        std::cout << " part = " << termcolor::bold << termcolor::bright_white << nstmPart << termcolor::reset << std::endl;
        std::cout << "  (STM+NSTM) / QA + bias = ";
        std::cout << termcolor::bright_white << "(" << stmPart;
        if (nstmPart < 0)
            std::cout << " - " << -nstmPart;
        else
            std::cout << " + " << nstmPart;
        std::cout << ") / " << NNUE::QA << " + " << bias;
        std::cout << " = " << biased << termcolor::reset << std::endl;
        std::cout << "  x Scale / (QA x QB) = " << termcolor::bright_white << biased << " x " << NNUE::SCALE;
        std::cout << " / (" << NNUE::QA << " x " << NNUE::QB << ") = " << rawEval << termcolor::reset << std::endl;
        std::cout << "  x material / 4096 = " << termcolor::bright_white << rawEval << " x " << materialScale << " / 4096";
        std::cout << " = " << (rawEval * materialScale / 4096) << termcolor::reset << std::endl;
    } else {
        std::cout << "  parts STM " << termcolor::bold << termcolor::bright_white << stmPart << termcolor::reset;
        std::cout << "  NSTM " << termcolor::bold << termcolor::bright_white << nstmPart << termcolor::reset;
        std::cout << "  bias " << termcolor::bold << termcolor::bright_white << bias << termcolor::reset;
        std::cout << "  -> raw " << termcolor::bold << termcolor::bright_white << rawEval << termcolor::reset;
        std::cout << "  x" << materialScale << "/4096" << std::endl;
    }
    if (full)
        std::cout << std::endl;

    auto humanShort = [](int64_t v) {
        std::ostringstream os;
        int64_t a = v < 0 ? -v : v;
        if (a >= 1000000)
            os << (v < 0 ? "-" : "") << std::fixed << std::setprecision(1) << (a / 1000000.0) << 'M';
        else if (a >= 1000)
            os << (v < 0 ? "-" : "") << std::fixed << std::setprecision(1) << (a / 1000.0) << 'k';
        else
            os << v;
        return os.str();
    };

    auto denseEntry = [&](int idx, int16_t act, int16_t w, int64_t contrib, int64_t maxAbs) {
        std::ostringstream os;
        int barLen = int((std::llabs(contrib) * 10 + maxAbs / 2) / maxAbs);
        os << ' ' << std::setw(4) << idx << ' ' << std::setw(4) << act << ' ' << std::setw(6) << w << ' ';
        if (contrib >= 0)
            os << termcolor::green;
        else
            os << termcolor::red;
        os << std::setw(7) << humanShort(contrib);
        for (int k = 0; k < barLen; k++) os << '#';
        for (int k = barLen; k < 10; k++) os << ' ';
        os << termcolor::reset;
        return os.str();
    };

    auto printTopPair = [&](const char* leftLabel, std::vector<std::tuple<int64_t,int,int16_t,int16_t>>& left,
                            const char* rightLabel, std::vector<std::tuple<int64_t,int,int16_t,int16_t>>& right, size_t count) {
        auto byAbs = [](const auto& x, const auto& y) { return std::get<0>(x) > std::get<0>(y); };
        size_t nl = std::min(count, left.size());
        size_t nr = std::min(count, right.size());
        size_t n = std::max(nl, nr);
        if (nl > 0) std::partial_sort(left.begin(), left.begin() + nl, left.end(), byAbs);
        if (nr > 0) std::partial_sort(right.begin(), right.begin() + nr, right.end(), byAbs);
        int64_t maxAbs = 1;
        if (nl > 0) maxAbs = std::max(maxAbs, std::get<0>(left[0]));
        if (nr > 0) maxAbs = std::max(maxAbs, std::get<0>(right[0]));
        std::cout << "  " << termcolor::bold << termcolor::bright_white << leftLabel << termcolor::reset;
        std::cout << std::string(36 - std::string(leftLabel).size(), ' ') << "   ";
        std::cout << termcolor::bold << termcolor::bright_white << rightLabel << termcolor::reset << std::endl;
        for (size_t i = 0; i < n; i++) {
            std::cout << "  ";
            if (i < nl) {
                int idx; int16_t act; int16_t w; int64_t ab;
                std::tie(ab, idx, act, w) = left[i];
                std::cout << denseEntry(idx, act, w, int64_t(act) * int16_t(act * w), maxAbs);
            } else {
                std::cout << std::string(36, ' ');
            }
            std::cout << "   ";
            if (i < nr) {
                int idx; int16_t act; int16_t w; int64_t ab;
                std::tie(ab, idx, act, w) = right[i];
                std::cout << denseEntry(idx, act, w, int64_t(act) * int16_t(act * w), maxAbs);
            }
            std::cout << std::endl;
        }
    };

    auto printTop = [&](const char* label, std::vector<std::tuple<int64_t,int,int16_t,int16_t>>& v, size_t count) {
        std::cout << "  Top " << label << " neurons" << termcolor::color<244> << " (idx act w contrib)" << termcolor::reset << std::endl;
        size_t n = std::min(count, v.size());
        std::partial_sort(v.begin(), v.begin() + n, v.end(),
            [](const auto& x, const auto& y) { return std::get<0>(x) > std::get<0>(y); });
        int64_t maxAbs = n > 0 ? std::get<0>(v[0]) : 1;
        for (size_t i = 0; i < n; i++) {
            int64_t ab;
            int idx;
            int16_t act;
            int16_t w;
            std::tie(ab, idx, act, w) = v[i];
            int64_t contrib = int64_t(act) * int16_t(act * w);
            int barLen = int((ab * 14 + maxAbs / 2) / maxAbs);
            std::cout << "    ";
            std::cout << termcolor::color<244> << "#" << termcolor::reset;
            std::cout << termcolor::bold << termcolor::bright_white << std::setw(5) << idx << termcolor::reset;
            std::cout << "  act " << termcolor::bright_white << std::setw(4) << act << termcolor::reset;
            std::cout << "  w " << std::setw(7) << w;
            std::cout << "  c ";
            if (contrib >= 0)
                std::cout << termcolor::green;
            else
                std::cout << termcolor::red;
            std::cout << std::setw(11) << contrib;
            for (int k = 0; k < barLen; k++) std::cout << '#';
            std::cout << termcolor::reset << std::endl;
        }
    };

    section("Top neurons");
    if (full) {
        printTop(stmIsWhite ? "White/STM" : "Black/STM", stmTop, 8);
        printTop(stmIsWhite ? "Black/NSTM" : "White/NSTM", nstmTop, 8);
    } else {
        printTopPair(stmIsWhite ? "STM White" : "STM Black", stmTop,
                     stmIsWhite ? "NSTM Black" : "NSTM White", nstmTop, 3);
        std::cout << termcolor::color<244> << "  (idx act w contrib, nnue full for top 8)" << termcolor::reset << std::endl;
    }
    if (full)
        std::cout << std::endl;

    section("Eval");
    {
        std::stringstream ss;
        if (isMateScore) {
            int mateIn = (SEARCH::MATE_SCORE - (std::abs(finalEval) - 1)) / 2;
            mateIn = finalEval < 0 ? mateIn * -1 : mateIn;
            ss << ((mateIn < 0) ? "-M" : "+M") << std::abs(mateIn);
            printGauge(mateIn > 0 ? 5.0 : -5.0, ss.str(), mateIn > 0, true);
        } else {
            ss << std::showpos << std::fixed << std::setprecision(2) << (normalized / 100.0) << std::noshowpos;
            printGauge(normalized / 100.0, ss.str(), normalized > 0, false);
        }
    }
    std::cout << "  Raw ";
    std::cout << termcolor::bold << termcolor::bright_white << rawEval << termcolor::reset;
    std::cout << "   Normalized ";
    if (isMateScore) {
        int mateIn = (SEARCH::MATE_SCORE - (std::abs(finalEval) - 1)) / 2;
        mateIn = finalEval < 0 ? mateIn * -1 : mateIn;
        if (mateIn > 0)
            std::cout << termcolor::bold << termcolor::bright_green;
        else
            std::cout << termcolor::bold << termcolor::bright_red;
        std::cout << ((mateIn < 0) ? "-M" : "+M") << std::abs(mateIn);
        std::cout << termcolor::reset;
    } else {
        if (normalized > 0)
            std::cout << termcolor::bold << termcolor::green;
        else if (normalized < 0)
            std::cout << termcolor::bold << termcolor::red;
        else
            std::cout << termcolor::bright_white;
        std::stringstream ss;
        ss << std::showpos << std::fixed << std::setprecision(2) << (normalized / 100.0) << std::noshowpos;
        std::cout << ss.str();
        std::cout << termcolor::reset;
    }
    std::cout << "   WDL " << termcolor::color<244>;
    std::cout << wdl.wins << "W " << wdl.draws << "D " << wdl.losses << "L";
    std::cout << termcolor::reset << std::endl;
    printWdlBar(wdl);
    std::cout << "  Final eval: ";
    if (finalEval > 0)
        std::cout << termcolor::bold << termcolor::green;
    else if (finalEval < 0)
        std::cout << termcolor::bold << termcolor::red;
    else
        std::cout << termcolor::bold << termcolor::bright_white;
    std::cout << finalEval << termcolor::reset << std::endl;
}

void Board::AddMove(Move move) {
    moveList[currentMoveIndex] = move;
    currentMoveIndex++;
}

void Board::ResetMoves() {
    currentMoveIndex = 0;
}

void Board::ListMoves() {
	int moveCount = 1;
	for (int i = 0; i < currentMoveIndex; i++) {
        if (!IsLegal(moveList[i])) continue;

		Board copy = *this;
		copy.MakeMove(moveList[i]);


		std::cout << moveCount << ". ";
		moveList[i].PrintMove();
		std::cout << "( " << moveTypes[moveList[i].GetFlags()] << " )";
		std::cout << std::endl;
		moveCount++;
	}
}

int Board::GetPieceType(int square) {
	return mailbox[square];
}

int Board::GetPieceColor(int square) {
	return (colors[Black].IsSet(square));
}

bool Board::InCheck() {
	Bitboard myKingSquare = colors[sideToMove] & pieces[King];

	return colorThreats[!sideToMove] & myKingSquare;
}

void Board::SetPiece(int piece, int square, bool color) {
	pieces[piece].SetBit(square);
	colors[color].SetBit(square);
	occupied.SetBit(square);

    mailbox[square] = piece;

    hashKey ^= UTILS::zKeys[color][piece][square];

    if (piece == Pawn) {
        pawnKey ^= UTILS::zKeys[color][Pawn][square];
    } else {
    	nonPawnKey ^= UTILS::zKeys[color][piece][square];

        if (piece == King || piece == Rook || piece == Queen) {
            majorKey ^= UTILS::zKeys[color][piece][square];
        }
    }
}

void Board::RemovePiece(int piece, int square, bool color) {
	pieces[piece].PopBit(square);
	colors[color].PopBit(square);
	occupied.PopBit(square);

    mailbox[square] = nullPieceType;

    hashKey ^= UTILS::zKeys[color][piece][square];

    if (piece == Pawn) {
        pawnKey ^= UTILS::zKeys[color][Pawn][square];
    } else {
    	nonPawnKey ^= UTILS::zKeys[color][piece][square];

        if (piece == King || piece == Rook || piece == Queen) {
            majorKey ^= UTILS::zKeys[color][piece][square];
        }
    }
}

static void UpdateCastlingRights(Board &board, int square, int type, int color) {
	if (type == Rook) {
        board.hashKey ^= UTILS::zCastle[board.castlingRights];
		int queenSideRook = color ? a8 : a1;
		int kingSideRook = color ? h8 : h1;

		if (square == queenSideRook) {
            board.castlingRights &= color ? ~blackQueenRight : ~whiteQueenRight;
		} else if (square == kingSideRook) {
            board.castlingRights &= color ? ~blackKingRight : ~whiteKingRight;
		}

        board.hashKey ^= UTILS::zCastle[board.castlingRights];
	} else if (type == King) {
        board.hashKey ^= UTILS::zCastle[board.castlingRights];

        board.castlingRights &= color ? ~blackKingRight : ~whiteKingRight;
        board.castlingRights &= color ? ~blackQueenRight : ~whiteQueenRight;

        board.hashKey ^= UTILS::zCastle[board.castlingRights];
	}
}

void Board::Promote(int square, int pieceType, int color, bool isCapture) {
	if (isCapture) {
		int targetType = GetPieceType(square);
		RemovePiece(targetType, square, !color);
		UpdateCastlingRights(*this, square, targetType, !color);
	}

	SetPiece(pieceType, square, color);
}

void Board::MakeMove(Move move) {
    const bool accParentClean = accUpdate.type == ACC::UpdateType::None;
    accUpdate = ACC::Update();

    if (!move) {
        if (accParentClean) accUpdate.type = ACC::UpdateType::Copy;
        else accUpdate.setRefresh();

		int newEpTarget = noEPTarget;

        sideToMove = !sideToMove;
        hashKey ^= UTILS::zSide;

        if (enPassantTarget != noEPTarget) {
			hashKey ^= UTILS::zEnPassant[enPassantTarget % 8];
        }

        enPassantTarget = newEpTarget;

        return;
    }

	int newEpTarget = noEPTarget;

	int attackerPiece = GetPieceType(move.MoveFrom());
	int attackerColor = GetPieceColor(move.MoveFrom());

	int targetPiece = GetPieceType(move.MoveTo());
	int direction = attackerColor ? south : north;

	int endPiece = attackerPiece;

    uint8_t refreshMask = 0;

    if (attackerPiece == King) {
        const int toFile = move.MoveTo() % 8;

        if (attackerColor == White) {
            if ((toFile > 3) != mirroredWhite) {
                mirroredWhite = !mirroredWhite;
                refreshMask |= 1 << White;
            }
        } else {
            if ((toFile > 3) != mirroredBlack) {
                mirroredBlack = !mirroredBlack;
                refreshMask |= 1 << Black;
            }
        }

        if (NNUE::kingBuckets[attackerColor][move.MoveFrom()] != NNUE::kingBuckets[attackerColor][move.MoveTo()]) {
            refreshMask |= 1 << attackerColor;
        }
    }

	RemovePiece(attackerPiece, move.MoveFrom(), attackerColor);

	if (move.IsPromo()) {
		endPiece = move.GetPromoPiece();
		if (move.IsCapture()) {
			Promote(move.MoveTo(), endPiece, sideToMove, true);
		} else {
			Promote(move.MoveTo(), endPiece, sideToMove, false);
		}
	}

    {
        if (move.IsCapture()) {
            if (move.GetFlags() != epCapture) {
                accUpdate.setAddSubSub(sideToMove, move.MoveTo(), endPiece, move.MoveFrom(), attackerPiece, move.MoveTo(), targetPiece);
            } else {
                accUpdate.setAddSubSub(sideToMove, enPassantTarget, endPiece, move.MoveFrom(), attackerPiece, move.MoveTo() - direction, Pawn);
            }
        } else {
            if (move.GetFlags() != kingCastle && move.GetFlags() != queenCastle) {
                accUpdate.setAddSub(sideToMove, move.MoveTo(), endPiece, move.MoveFrom(), attackerPiece);
            }
        }
    }

	switch (move.GetFlags())
	{
	case quiet:
		SetPiece(attackerPiece, move.MoveTo(), attackerColor);
		break;
	case doublePawnPush:
		SetPiece(attackerPiece, move.MoveTo(), attackerColor);
		newEpTarget = move.MoveFrom() + direction;
		break;
	case capture:
		RemovePiece(targetPiece, move.MoveTo(), !attackerColor);
		SetPiece(attackerPiece, move.MoveTo(), attackerColor);

		UpdateCastlingRights(*this, move.MoveTo(), targetPiece, !attackerColor);

		break;
	case epCapture:
		RemovePiece(Pawn, move.MoveTo() - direction, !attackerColor);
		SetPiece(attackerPiece, move.MoveTo(), attackerColor);
		break;
	case kingCastle:
		{
			int rookSquare = attackerColor ? h8 : h1;

			RemovePiece(Rook, rookSquare, attackerColor);

			SetPiece(Rook, rookSquare - 2, attackerColor);

			SetPiece(attackerPiece, move.MoveTo(), attackerColor);

            {
                accUpdate.setAddAddSubSub(sideToMove, move.MoveTo(), King, rookSquare - 2, Rook, move.MoveFrom(), King, rookSquare, Rook);
            }

			break;
		}
	case queenCastle:
		{
			int rookSquare = attackerColor ? a8 : a1;

			RemovePiece(Rook, rookSquare, attackerColor);

			SetPiece(Rook, rookSquare + 3, attackerColor);

			SetPiece(attackerPiece, move.MoveTo(), attackerColor);

            {
                accUpdate.setAddAddSubSub(sideToMove, move.MoveTo(), King, rookSquare + 3, Rook, move.MoveFrom(), King, rookSquare, Rook);
            }

            break;
		}
	default:
		break;
	}

    if (!accParentClean || accUpdate.type == ACC::UpdateType::None) {
        accUpdate.setRefresh();
    } else {
        accUpdate.refreshMask = refreshMask;
    }

	UpdateCastlingRights(*this, move.MoveFrom(), attackerPiece, attackerColor);

	sideToMove = !attackerColor;
    hashKey ^= UTILS::zSide;

    if (enPassantTarget != noEPTarget) {
        hashKey ^= UTILS::zEnPassant[enPassantTarget % 8];
    }

    if (newEpTarget != noEPTarget) {
        hashKey ^= UTILS::zEnPassant[newEpTarget % 8];
    }

	enPassantTarget = newEpTarget;

	MOVEGEN::GenThreatMaps(*this);
    pinned = {CalcPinned(White), CalcPinned(Black)};
    checkers = CalcCheckers();
    CalcCheckZones();


	if (attackerColor == Black) fullMoves++;
	if (attackerPiece == Pawn || move.IsCapture()) {
		halfMoves = 0;
	} else {
		halfMoves++;
	}

    positionIndex++;
}

bool Board::InPossibleZug() {
    Bitboard toCheck;

    for (int piece = Knight; piece <= Queen; piece++) {
        toCheck |= (pieces[piece] & colors[sideToMove]);
    }

    return !toCheck;
}

ACC::BucketPair Board::GetBuckets() {
    int wKingSq = (pieces[King] & colors[White]).getLS1BIndex();
    int bKingSq = (pieces[King] & colors[Black]).getLS1BIndex();

    return {NNUE::kingBuckets[White][wKingSq],NNUE::kingBuckets[Black][bKingSq]};
}

void Board::RefreshAccumulator(ACC::AccumulatorPair& out) const {
	const int wKingSq = (pieces[King] & colors[White]).getLS1BIndex();
	const int bKingSq = (pieces[King] & colors[Black]).getLS1BIndex();
	const ACC::BucketPair bucketPair = {NNUE::kingBuckets[White][wKingSq], NNUE::kingBuckets[Black][bKingSq]};

	out.white = NNUE::net.accumulator_biases;
	out.black = NNUE::net.accumulator_biases;

	for (int color = White; color <= Black; color++) {
		Bitboard bb = colors[color];

		while (bb) {
			int square = bb.getLS1BIndex();

			int wInput = ACC::CalculateIndex(White, color, mailbox[square], square, mirroredWhite);
			int bInput = ACC::CalculateIndex(Black, color, mailbox[square], square, mirroredBlack);

			ACC::AddRow(out.white, &NNUE::net.accumulator_weights[bucketPair.white][wInput * NNUE::HL_SIZE]);
			ACC::AddRow(out.black, &NNUE::net.accumulator_weights[bucketPair.black][bInput * NNUE::HL_SIZE]);

			bb.PopBit(square);
		}
	}
}

void Board::RefreshPerspective(ACC::FinnyTable& finny, bool perspective, ACC::Accumulator& out) const {
	const bool mirrored = perspective == White ? mirroredWhite : mirroredBlack;
	const int kingSq = (pieces[King] & colors[perspective]).getLS1BIndex();
	const int bucket = NNUE::kingBuckets[perspective][kingSq];

	ACC::FinnyEntry& entry = finny.entries[perspective][mirrored][bucket];

	const int16_t* adds[32];
	const int16_t* subs[32];
	int nAdds = 0;
	int nSubs = 0;

	for (int color = White; color <= Black; color++) {
		for (int pt = Pawn; pt <= King; pt++) {
			Bitboard tmp = pieces[pt] & colors[color];
			const uint64_t now = tmp;
			const uint64_t old = entry.bb[color][pt];

			uint64_t added = now & ~old;
			uint64_t removed = old & ~now;

			while (added) {
				const int square = std::countr_zero(added);
				added &= added - 1;
				adds[nAdds++] = ACC::Row(bucket, ACC::CalculateIndex(perspective, color, pt, square, mirrored));
			}

			while (removed) {
				const int square = std::countr_zero(removed);
				removed &= removed - 1;
				subs[nSubs++] = ACC::Row(bucket, ACC::CalculateIndex(perspective, color, pt, square, mirrored));
			}

			entry.bb[color][pt] = now;
		}
	}

	ACC::ApplyInPlace(entry.acc, adds, nAdds, subs, nSubs);
	out = entry.acc;
}

void Board::ApplyDeltaPerspective(bool perspective, ACC::Accumulator& dst, const ACC::Accumulator& src) const {
	const bool mirrored = perspective == White ? mirroredWhite : mirroredBlack;
	const int kingSq = (pieces[King] & colors[perspective]).getLS1BIndex();
	const int bucket = NNUE::kingBuckets[perspective][kingSq];

	const bool stm = accUpdate.stm;

	const int16_t* a1 = ACC::Row(bucket, ACC::CalculateIndex(perspective, stm, accUpdate.addPT1, accUpdate.add1, mirrored));
	const int16_t* a2 = nullptr;
	const int16_t* s1 = ACC::Row(bucket, ACC::CalculateIndex(perspective, stm, accUpdate.subPT1, accUpdate.sub1, mirrored));
	const int16_t* s2 = nullptr;

	if (accUpdate.nAdd == 2)
		a2 = ACC::Row(bucket, ACC::CalculateIndex(perspective, stm, accUpdate.addPT2, accUpdate.add2, mirrored));

	if (accUpdate.nSub == 2)
		s2 = ACC::Row(bucket, ACC::CalculateIndex(perspective, accUpdate.sub2Opp ? !stm : stm, accUpdate.subPT2, accUpdate.sub2, mirrored));

	ACC::Apply(dst, src, a1, a2, s1, s2);
}

void Board::UpdateAccumulator(ACC::AccStack& stack, int ply) {
	using ACC::UpdateType;

	if (accUpdate.type == UpdateType::None) return;

	if (accUpdate.type == UpdateType::Copy && ply > 0) {
		stack.cur[ply] = stack.cur[ply - 1];
		accUpdate.type = UpdateType::None;
		return;
	}

	ACC::AccumulatorPair& dst = stack.slots[ply];

	if (ply == 0 || accUpdate.type != UpdateType::Delta) {
		RefreshPerspective(stack.finny, White, dst.white);
		RefreshPerspective(stack.finny, Black, dst.black);
	} else {
		const ACC::AccumulatorPair& prev = *stack.cur[ply - 1];

		for (int perspective = White; perspective <= Black; perspective++) {
			if (accUpdate.refreshMask & (1 << perspective))
				RefreshPerspective(stack.finny, perspective, dst.get(perspective));
			else
				ApplyDeltaPerspective(perspective, dst.get(perspective), prev.get(perspective));
		}
	}

	stack.cur[ply] = &dst;
	accUpdate.type = UpdateType::None;
}

Bitboard Board::AttacksTo(int square, Bitboard occupancy) {
    Bitboard attacks;


    attacks |= (MOVEGEN::pawnAttacks[sideToMove][square] & colors[!sideToMove] & pieces[Pawn]);

    attacks |= (MOVEGEN::pawnAttacks[!sideToMove][square] & colors[sideToMove] & pieces[Pawn]);

    attacks |= (MOVEGEN::knightAttacks[square] & pieces[Knight]);

    attacks |= (MOVEGEN::kingAttacks[square] & pieces[King]);

    Bitboard bishopAttacks = MOVEGEN::getBishopAttack(square, occupancy);
    Bitboard rookAttacks = MOVEGEN::getRookAttack(square, occupancy);

    attacks |= (rookAttacks & (pieces[Rook] | pieces[Queen]));

    attacks |= (bishopAttacks & (pieces[Bishop] | pieces[Queen]));

    return attacks;
}

static Bitboard rayBetween(int sq1, int sq2) {
    if (sq1 == sq2) return 0ULL;

    int f1 = sq1 % 8, r1 = sq1 / 8;
    int f2 = sq2 % 8, r2 = sq2 / 8;

    int df = (f2 > f1) - (f2 < f1);
    int dr = (r2 > r1) - (r2 < r1);

    if (df != 0 && dr != 0 && std::abs(f2 - f1) != std::abs(r2 - r1))
        return 0ULL;

    Bitboard ray = 0ULL;

    int f = f1 + df;
    int r = r1 + dr;

    while (f != f2 || r != r2) {
        ray |= 1ULL << (r * 8 + f);
        f += df;
        r += dr;
    }

    ray &= ~((1ULL << sq1) | (1ULL << sq2));

    return ray;
}

static Bitboard fullRay(int from, int to) {
    int df = (to % 8) - (from % 8);
    int dr = (to / 8) - (from / 8);

    if (df != 0) df /= std::abs(df);
    if (dr != 0) dr /= std::abs(dr);

    Bitboard ray = 0ULL;

    int f = from % 8 + df;
    int r = from / 8 + dr;
    while (f >= 0 && f < 8 && r >= 0 && r < 8) {
        ray |= 1ULL << (r * 8 + f);
        f += df;
        r += dr;
    }

    f = from % 8 - df;
    r = from / 8 - dr;
    while (f >= 0 && f < 8 && r >= 0 && r < 8) {
        ray |= 1ULL << (r * 8 + f);
        f -= df;
        r -= dr;
    }

    ray |= 1ULL << from;
    return ray;
}

bool Board::IsSquareThreatened(bool side, int square) {
	return colorThreats[!side].IsSet(square);
}

Bitboard Board::CalcCheckers() {
    return AttacksTo((colors[sideToMove] & pieces[King]).getLS1BIndex(), occupied) & colors[!sideToMove];
}

Bitboard Board::CalcPinned(bool color) {
    Bitboard pinned;

    int kingSquare = (pieces[King] & colors[color]).getLS1BIndex();

    assert(kingSquare != -1);

    Bitboard oppQueens = pieces[Queen] & colors[!color];

    Bitboard potentialAttackers = MOVEGEN::getBishopAttack(kingSquare, colors[!color]) & (oppQueens | (pieces[Bishop] & colors[!color]))
                                | MOVEGEN::getRookAttack(kingSquare, colors[!color]) & (oppQueens | (pieces[Rook] & colors[!color]));

    while (potentialAttackers) {
        int attackerSquare = potentialAttackers.getLS1BIndex();

        Bitboard isPinned = colors[color] & rayBetween(attackerSquare, kingSquare);

        if (isPinned.PopCount() == 1)
            pinned |= isPinned;

        potentialAttackers.PopBit(attackerSquare);
    }

    return pinned;
}

bool Board::IsLegal(Move &move) {
    assert(move != 0);

    int us = sideToMove;
    int them = !us;

    int from = move.MoveFrom();
    int to = move.MoveTo();

    Bitboard king = pieces[King] & colors[us];
    int kingSquare = king.getLS1BIndex();

    int moveType = move.GetFlags();

    if (moveType == epCapture) {
        int epCapturedSquare = to + (us == White ? -8 : 8);
        Bitboard occAfterEP = occupied ^ Bitboard::GetSquare(from) ^ Bitboard::GetSquare(to)
                                ^ Bitboard::GetSquare(epCapturedSquare);

        Bitboard theirQueens = pieces[Queen] & colors[them];

        return (MOVEGEN::getBishopAttack(kingSquare, occAfterEP) & (theirQueens | (pieces[Bishop] & colors[them]))).PopCount() < 1
            && (MOVEGEN::getRookAttack(kingSquare, occAfterEP) & (theirQueens | (pieces[Rook] & colors[them]))).PopCount() < 1;
    }

    int movingPiece = GetPieceType(from);

    if (movingPiece == King) {
        Bitboard kinglessOcc = occupied ^ king;
        Bitboard theirQueens = pieces[Queen] & colors[them];

        return !colorThreats[them].IsSet(to)
            && (MOVEGEN::getBishopAttack(to, kinglessOcc) & (theirQueens | (pieces[Bishop] & colors[them]))).PopCount() == 0
            && (MOVEGEN::getRookAttack(to, kinglessOcc) & (theirQueens | (pieces[Rook] & colors[them]))).PopCount() == 0;
    }

    if (checkers.PopCount() > 1) {
        return false;
    }

    if (pinned[us].IsSet(from) && !fullRay(kingSquare, from).IsSet(to))
        return false;

    if (checkers.PopCount() < 1)
        return true;

    int checkerSquare = checkers.getLS1BIndex();

    return (rayBetween(kingSquare, checkerSquare) | checkers).IsSet(to);
}

bool Board::IsPseudoLegal(Move &move) {
    if (!move) return true;

    const int from = move.MoveFrom();
    const int to   = move.MoveTo();
    const int flag = move.GetFlags();

    if ((unsigned)from >= 64u || (unsigned)to >= 64u) return false;

    const int us   = sideToMove;
    const int them = !sideToMove;

    const int movingPiece = GetPieceType(from);
    if (movingPiece == nullPieceType) return false;
    if (!colors[us].IsSet(from)) return false;
    if (colors[us].IsSet(to)) return false;

    const bool isCapture = move.IsCapture();
    const int targetPiece = GetPieceType(to);

    if (flag == capture) {
        if (targetPiece == nullPieceType) return false;
        if (!colors[them].IsSet(to)) return false;
    } else if (isCapture && flag != epCapture) {
        if (targetPiece == nullPieceType) return false;
        if (!colors[them].IsSet(to)) return false;
    } else if (!isCapture) {
        if (occupied.IsSet(to)) {
            if (flag != kingCastle && flag != queenCastle) return false;
        }
    }

    const int fromFile = from & 7;
    const int toFile   = to & 7;
    const int fromRank = from >> 3;
    const int toRank   = to >> 3;

    const int dir = us ? south : north;

    auto isOneStepPawnPush = [&] {
        return to == from + dir && !occupied.IsSet(to);
    };

    auto isPawnCaptureTo = [&] {
        return (to == from + dir - 1 && toFile == fromFile - 1)
            || (to == from + dir + 1 && toFile == fromFile + 1);
    };

    switch (flag) {
    case quiet:
    case capture:
        break;

    case doublePawnPush: {
        if (movingPiece != Pawn) return false;
        if (!us) {
            if (fromRank != 1) return false;
        } else {
            if (fromRank != 6) return false;
        }
        const int mid = from + dir;
        if (to != from + 2 * dir) return false;
        if (occupied.IsSet(mid) || occupied.IsSet(to)) return false;
        return true;
    }

    case epCapture: {
        if (movingPiece != Pawn) return false;
        if (enPassantTarget == noEPTarget) return false;
        if (to != enPassantTarget) return false;
        if (!isPawnCaptureTo()) return false;

        const int capSq = to - dir;
        if (!colors[them].IsSet(capSq)) return false;
        if (GetPieceType(capSq) != Pawn) return false;
        if (occupied.IsSet(to)) return false;
        return true;
    }

    case kingCastle:
    case queenCastle: {
        if (movingPiece != King) return false;

        const int kingSquare = (colors[us] & pieces[King]).getLS1BIndex();
        if (from != kingSquare) return false;

        const int kingRight  = us ? blackKingRight  : whiteKingRight;
        const int queenRight = us ? blackQueenRight : whiteQueenRight;

        if (flag == kingCastle) {
            if (!(castlingRights & kingRight)) return false;
            const U64 KingSide = us ? 0xf000000000000000ULL : 0xf0ULL;
            const U64 mask     = us ? 0x7000000000000000ULL : 0x70ULL;
            const int targetSq = us ? g8 : g1;
            if (to != targetSq) return false;

            if ((Bitboard(KingSide) & occupied).PopCount() != 2) return false;
            if (mask & colorThreats[them]) return false;
            return true;
        } else {
            if (!(castlingRights & queenRight)) return false;
            const U64 QueenSide = us ? 0x1f00000000000000ULL : 0x1fULL;
            const U64 mask      = us ? 0x1c00000000000000ULL : 0x1cULL;
            const int targetSq  = us ? c8 : c1;
            if (to != targetSq) return false;

            if ((Bitboard(QueenSide) & occupied).PopCount() != 2) return false;
            if (mask & colorThreats[them]) return false;
            return true;
        }
    }

    default:
        if (!move.IsPromo()) return false;
        if (movingPiece != Pawn) return false;

        if (!us) {
            if (toRank != 7) return false;
        } else {
            if (toRank != 0) return false;
        }

        if (move.IsCapture()) {
            if (!isPawnCaptureTo()) return false;
            if (!colors[them].IsSet(to)) return false;
        } else {
            if (!isOneStepPawnPush()) return false;
        }
        return true;
    }

    switch (movingPiece) {
    case Pawn: {
        if (!us) { if (toRank == 7) return false; }
        else     { if (toRank == 0) return false; }

        if (move.IsCapture()) {
            if (!isPawnCaptureTo()) return false;
            return true;
        } else {
            return isOneStepPawnPush();
        }
    }

    case Knight:
        return (MOVEGEN::knightAttacks[from] & Bitboard::GetSquare(to)) != 0;

    case Bishop: {
        Bitboard att = MOVEGEN::getBishopAttack(from, occupied);
        return (att & Bitboard::GetSquare(to)) != 0;
    }

    case Rook: {
        Bitboard att = MOVEGEN::getRookAttack(from, occupied);
        return (att & Bitboard::GetSquare(to)) != 0;
    }

    case Queen: {
        Bitboard att = MOVEGEN::getBishopAttack(from, occupied) | MOVEGEN::getRookAttack(from, occupied);
        return (att & Bitboard::GetSquare(to)) != 0;
    }

    case King:
        return (MOVEGEN::kingAttacks[from] & Bitboard::GetSquare(to)) != 0;

    default:
        return false;
    }
}

void Board::CalcCheckZones() {
    int oppKingSquare = (colors[!sideToMove] & pieces[King]).getLS1BIndex();

	checkZones[0] = MOVEGEN::pawnAttacks[!sideToMove][oppKingSquare];
    checkZones[1] = MOVEGEN::knightAttacks[oppKingSquare];
    checkZones[2] = MOVEGEN::getBishopAttack(oppKingSquare, occupied);
    checkZones[3] = MOVEGEN::getRookAttack(oppKingSquare, occupied);
}

bool Board::GivesDirectCheck(Move &move) {
    int attackerType = nullPieceType;

    if (move.IsPromo()) {
        attackerType = move.GetPromoPiece();
    } else {
        attackerType = GetPieceType(move.MoveFrom());
    }

    if (attackerType == King)
        return false;

    Bitboard checkZone = 0ULL;

    if (attackerType == Queen) {
        checkZone = checkZones[Bishop] | checkZones[Rook];
    } else {
        checkZone = checkZones[attackerType];
    }

    return checkZone.IsSet(move.MoveTo());
}
