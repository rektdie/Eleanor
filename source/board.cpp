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
	sideToMove = White;
	occupied = 0ULL;

	pieces = std::array<Bitboard, 6>();
	colors = std::array<Bitboard, 2>();
    threatMaps = std::array<std::array<U64, 64>, 2>();

    moveList = std::array<Move, 218>();

    currentMoveIndex = 0;

    hashKey = 0ULL;
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

    occupied = colors[White] | colors[Black];
    hashKey = UTILS::GetHashKey(*this);
	MOVEGEN::GenerateMoves<All>(*this);
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
}

void Board::AddMove(Move move) {
    moveList[currentMoveIndex] = move;
    currentMoveIndex++;
}

void Board::ResetMoves() {
    currentMoveIndex = 0;
}

U64 Board::GetThreatMaps(bool side) {
    U64 combined = 0ULL;
	for (int type = Pawn; type <= King; type++) {
		combined |= threatMaps[side][type];
	}
	return combined;
}

void Board::ListMoves() {
	for (int i = 0; i < currentMoveIndex; i++) {
		std::cout << i+1 << ". ";
		moveList[i].PrintMove();
		std::cout << "( " << moveTypes[moveList[i].GetFlags()] << " )";
		std::cout << std::endl;
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
	int kingSquare = (pieces[King] & colors[sideToMove]).getLS1BIndex();

	for (int type = Pawn; type <= Queen; type++) {
		Bitboard moves = MOVEGEN::getPieceAttacks(kingSquare, type, sideToMove, occupied);

		if (moves & colors[!sideToMove] & pieces[type]) return true;
	}

	return false;
}

bool Board::IsLegal(Move move) {
	Board copy = *this;
	copy.MakeMove(move);
	copy.sideToMove = sideToMove;
	positionIndex--;

	return !copy.InCheck();
}

void Board::SetPiece(int piece, int square, bool color) {
	pieces[piece].SetBit(square);
	colors[color].SetBit(square);
	occupied.SetBit(square);

    hashKey ^= UTILS::zKeys[color][piece][square];
}

void Board::RemovePiece(int piece, int square, bool color) {
	pieces[piece].PopBit(square);
	colors[color].PopBit(square);
	occupied.PopBit(square);

    hashKey ^= UTILS::zKeys[color][piece][square];
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

void Board::MakeMove(Move move) {
	SEARCH::nodes++;
	
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
		
        return;
    }
	
	int newEpTarget = noEPTarget;

	int attackerPiece = GetPieceType(move.MoveFrom());
	int attackerColor = GetPieceColor(move.MoveFrom());

	int targetPiece = GetPieceType(move.MoveTo());
	int direction = attackerColor ? south : north;

	// Removing attacker piece from old position
	RemovePiece(attackerPiece, move.MoveFrom(), attackerColor);

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
			break;
		}
	case knightPromotion:
		Promote(move.MoveTo(), Knight, attackerColor, false);
		break;
	case bishopPromotion:
		Promote(move.MoveTo(), Bishop, attackerColor, false);
		break;
	case rookPromotion:
		Promote(move.MoveTo(), Rook, attackerColor, false);
		break;
	case queenPromotion:
		Promote(move.MoveTo(), Queen, attackerColor, false);
		break;
	case knightPromoCapture:
		Promote(move.MoveTo(), Knight, attackerColor, true);
		break;
	case bishopPromoCapture:
		Promote(move.MoveTo(), Bishop, attackerColor, true);
		break;
	case rookPromoCapture:
		Promote(move.MoveTo(), Rook, attackerColor, true);
		break;
	case queenPromoCapture:
		Promote(move.MoveTo(), Queen, attackerColor, true);
		break;
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
	
    positionIndex++;
    positionHistory[positionIndex] = hashKey;
}

bool Board::InPossibleZug() {
    Bitboard toCheck;

    // For all pieces other than pawns and king
    for (int piece = Knight; piece <= Queen; piece++) {
        toCheck |= (pieces[piece] & colors[sideToMove]);
    }

    return !toCheck;
}
