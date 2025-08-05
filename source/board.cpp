#include "board.h"
#include "movegen.h"
#include "tt.h"
#include <iostream>
#include <ranges>
#include <string_view>
#include "utils.h"

void Board::Reset() {
    castlingRights = 0;
	enPassantTarget = noEPTarget;

	halfMoves = 0;
	fullMoves = 1;

	sideToMove = White;
	occupied = 0ULL;

	pieces = std::array<Bitboard, 6>();
	colors = std::array<Bitboard, 2>();

	accPair = ACC::AccumulatorPair();

	pieceThreats = std::array<Bitboard, 6>();
	colorThreats = std::array<Bitboard, 2>();

    moveList = std::array<Move, MAX_MOVES>();

	positionIndex = 0;

    currentMoveIndex = 0;

    hashKey = 0ULL;
    pawnKey = 0ULL;
}

void Board::SetByFen(std::string_view fen) {
	Reset();

	// Starting from top left
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

			colors[side].SetBit(currSquare);
			pieces[pieceType].SetBit(currSquare);

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

	// full and half move
	if (tokens.size() > 4) {
		halfMoves = std::stoi(tokens[4]);
		fullMoves = std::stoi(tokens[5]);
	}

    occupied = colors[White] | colors[Black];
    hashKey = UTILS::GetHashKey(*this);
	MOVEGEN::GenThreatMaps(*this);
	MOVEGEN::GenerateMoves<All>(*this);
	ResetAccPair();
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

void Board::PrintNNUE() {
	std::cout << "Final eval: " << NNUE::net.Evaluate(*this) << std::endl;
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
		Board copy = *this;
		bool isLegal = copy.MakeMove(moveList[i]);

		if (!isLegal)
			continue;

		std::cout << moveCount << ". ";
		moveList[i].PrintMove();
		std::cout << "( " << moveTypes[moveList[i].GetFlags()] << " )";
		std::cout << std::endl;
		moveCount++;
	}
}

int Board::GetPieceType(int square) {
	for (int piece = Pawn; piece <= King; piece++) {
		if (pieces[piece].IsSet(square)) return piece;
	}

	return nullPieceType;
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

    hashKey ^= UTILS::zKeys[color][piece][square];

    if (piece == Pawn) {
        pawnKey ^= UTILS::zKeys[color][Pawn][square];
    }
}

void Board::RemovePiece(int piece, int square, bool color) {
	pieces[piece].PopBit(square);
	colors[color].PopBit(square);
	occupied.PopBit(square);

    hashKey ^= UTILS::zKeys[color][piece][square];

    if (piece == Pawn) {
        pawnKey ^= UTILS::zKeys[color][Pawn][square];
    }
}

// Updates castling rights
static void UpdateCastlingRights(Board &board, int square, int type, int color) {
	if (type == Rook) {
        // Removing old rights
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
        // Removing old rights
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

bool Board::MakeMove(Move move) {
	Board save = *this;
	// Null Move
    if (!move) {
		int newEpTarget = noEPTarget;

        sideToMove = !sideToMove;
        hashKey ^= UTILS::zSide;

        if (enPassantTarget != noEPTarget) {
			hashKey ^= UTILS::zEnPassant[enPassantTarget % 8];
        }

        if (newEpTarget != noEPTarget) {
			hashKey ^= UTILS::zEnPassant[newEpTarget % 8];
        }

        enPassantTarget = newEpTarget;

        return true;
    }

	int newEpTarget = noEPTarget;

	int attackerPiece = GetPieceType(move.MoveFrom());
	int attackerColor = GetPieceColor(move.MoveFrom());

	int targetPiece = GetPieceType(move.MoveTo());
	int direction = attackerColor ? south : north;

	int endPiece = attackerPiece;

	// Removing attacker piece from old position
	RemovePiece(attackerPiece, move.MoveFrom(), attackerColor);

	if (move.IsPromo()) {
		if (move.IsCapture()) {
			endPiece = move.GetFlags() - 9;
			Promote(move.MoveTo(), endPiece, sideToMove, true);
		} else {
			endPiece = move.GetFlags() - 5;
			Promote(move.MoveTo(), endPiece, sideToMove, false);
		}
	}

	if (move.IsCapture()) {
		if (move.GetFlags() != epCapture) {
			accPair.addSubSub(sideToMove, move.MoveTo(), endPiece, move.MoveFrom(), attackerPiece, move.MoveTo(), targetPiece);
		} else {
			accPair.addSubSub(sideToMove, enPassantTarget, endPiece, move.MoveFrom(), attackerPiece, move.MoveTo() - direction, Pawn);
		}
	} else {
		if (move.GetFlags() != kingCastle && move.GetFlags() != queenCastle) {
			accPair.addSub(sideToMove, move.MoveTo(), endPiece, move.MoveFrom(), attackerPiece);
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

			// Removing rook from old position
			RemovePiece(Rook, rookSquare, attackerColor);

			// Setting rook on new position
			SetPiece(Rook, rookSquare - 2, attackerColor);

			SetPiece(attackerPiece, move.MoveTo(), attackerColor);

			accPair.addAddSubSub(sideToMove, move.MoveTo(), King, rookSquare - 2, Rook, move.MoveFrom(), King, rookSquare, Rook);
			break;
		}
	case queenCastle:
		{
			int rookSquare = attackerColor ? a8 : a1;

			// Removing rook from old position
			RemovePiece(Rook, rookSquare, attackerColor);

			// Setting rook on new position
			SetPiece(Rook, rookSquare + 3, attackerColor);

			SetPiece(attackerPiece, move.MoveTo(), attackerColor);
			accPair.addAddSubSub(sideToMove, move.MoveTo(), King, rookSquare + 3, Rook, move.MoveFrom(), King, rookSquare, Rook);
			break;
		}
	default:
		break;
	}

	// Removing the right to castle on king and rook movement
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

    sideToMove = !sideToMove;
    if (InCheck())  {
		*this = save;
		return false;
	}
    sideToMove = !sideToMove;

	if (attackerColor == Black) fullMoves++;
	if (attackerPiece == Pawn || move.IsCapture()) {
		halfMoves = 0;
	} else {
		halfMoves++;
	}

    positionIndex++;

    return true;
}

bool Board::InPossibleZug() {
    Bitboard toCheck;

    // For all pieces other than pawns and king
    for (int piece = Knight; piece <= Queen; piece++) {
        toCheck |= (pieces[piece] & colors[sideToMove]);
    }

    return !toCheck;
}

void Board::ResetAccPair() {
	Bitboard whitePieces = colors[White];
	Bitboard blackPieces = colors[Black];

	accPair.white = NNUE::net.accumulator_biases;
	accPair.black = NNUE::net.accumulator_biases;

	while (whitePieces) {
		int square = whitePieces.getLS1BIndex();

		int wInput = ACC::CalculateIndex(White, White, GetPieceType(square), square);
		int bInput = ACC::CalculateIndex(Black, White, GetPieceType(square), square);

		for (int i = 0; i < NNUE::HL_SIZE; i++) {
			accPair.white[i] += NNUE::net.accumulator_weights[wInput * NNUE::HL_SIZE + i];
			accPair.black[i] += NNUE::net.accumulator_weights[bInput * NNUE::HL_SIZE + i];
		}

		whitePieces.PopBit(square);
	}

	while (blackPieces) {
		int square = blackPieces.getLS1BIndex();

		int wInput = ACC::CalculateIndex(White, Black, GetPieceType(square), square);
		int bInput = ACC::CalculateIndex(Black, Black, GetPieceType(square), square);

		for (int i = 0; i < NNUE::HL_SIZE; i++) {
			accPair.white[i] += NNUE::net.accumulator_weights[wInput * NNUE::HL_SIZE + i];
			accPair.black[i] += NNUE::net.accumulator_weights[bInput * NNUE::HL_SIZE + i];
		}

		blackPieces.PopBit(square);
	}
}

Bitboard Board::AttacksTo(int square, Bitboard occupancy) {
    Bitboard attacks;

    // Pawn attacks

    // Opponent attacks
    attacks |= (MOVEGEN::pawnAttacks[sideToMove][square] & colors[!sideToMove] & pieces[Pawn]);

    // Own piece attacks
    attacks |= (MOVEGEN::pawnAttacks[!sideToMove][square] & colors[sideToMove] & pieces[Pawn]);

    // Knight
    attacks |= (MOVEGEN::knightAttacks[square] & pieces[Knight]);

    // King
    attacks |= (MOVEGEN::kingAttacks[square] & pieces[King]);

    Bitboard bishopAttacks = MOVEGEN::getBishopAttack(square, occupancy);
    Bitboard rookAttacks = MOVEGEN::getRookAttack(square, occupancy);

    // Rook or queen
    attacks |= (rookAttacks & (pieces[Rook] | pieces[Queen]));

    // Bishop or queen
    attacks |= (bishopAttacks & (pieces[Bishop] | pieces[Queen]));

    return attacks;
}
