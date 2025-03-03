#include "board.h"
#include "movegen.h"
#include <iostream>
#include "tt.h"

void Board::Init() {
	SetByFen(StartingFen);
}

void Board::Reset() {
    castlingRights = 0;
	enPassantTarget = -1;
	halfMoves = 0;
	fullMoves = 0;
	sideToMove = White;
	occupied = 0ULL;

	pieces = std::array<Bitboard, 6>();
	colors = std::array<Bitboard, 2>();

    moveList = std::array<Move, 218>();
    currentMoveIndex = 0;

    hashKey = 0ULL;
}

void Board::SetByFen(const char* fen) {
	Reset();

	// Starting from top left
	Bitboard currentSquare = 0x100000000000000;

	int digits = 0;
	while (*fen != ' ' && *fen) {
		PIECE pieceType = Pawn;
		COLOR pieceColor = Black;

		switch (*fen) {
		case 'p':
			pieceType = Pawn;
			pieceColor = Black;
			break;
		case 'P':
			pieceType = Pawn;
			pieceColor = White;
			break;
		case 'n':
			pieceType = Knight;
			pieceColor = Black;
			break;
		case 'N':
			pieceType = Knight;
			pieceColor = White;
			break;
		case 'b':
			pieceType = Bishop;
			pieceColor = Black;
			break;
		case 'B':
			pieceType = Bishop;
			pieceColor = White;
			break;
		case 'r':
			pieceType = Rook;
			pieceColor = Black;
			break;
		case 'R':
			pieceType = Rook;
			pieceColor = White;
			break;
		case 'k':
			pieceType = King;
			pieceColor = Black;
			break;
		case 'K':
			pieceType = King;
			pieceColor = White;
			break;
		case 'q':
			pieceType = Queen;
			pieceColor = Black;
			break;
		case 'Q':
			pieceType = Queen;
			pieceColor = White;
			break;
		case '/':
			currentSquare = currentSquare >> (8 + digits);
			digits = 0;
			fen++;
			continue;
		default:
			int num = (*fen) - 48;
			fen++;
			if (*fen != '/') {
				currentSquare = currentSquare << num;
				digits += num;
			}
			continue;
		}

		pieces[pieceType] |= currentSquare;
		colors[pieceColor] |= currentSquare;

		fen++;
		if (*fen != '/') {
			digits++;
			currentSquare = currentSquare << 1;
		}
	}

	fen++; // skipping previous space
	int section = 0;

	while (*fen) {
		switch (*fen) {
		case ' ':
			fen++;
			section++;
			continue;
		case 'w':
			if (section == 0) {
				sideToMove = White;
			}
			break;
		case 'b':
			if (section == 0) {
				sideToMove = Black;
			}
			break;
		case 'K':
			castlingRights |= whiteKingRight;
			break;
		case 'Q':
			castlingRights |= whiteQueenRight;
			break;
		case 'k':
			castlingRights |= blackKingRight;
			break;
		case 'q':
			castlingRights |= blackQueenRight;
			break;
		default:
			if (*fen != '-') {
				if (section == 2) {
					int passantFile;
					switch (*fen)
					{
					case 'a':
						passantFile = 0;
						break;
					case 'b':
						passantFile = 1;
						break;
					case 'c':
						passantFile = 2;
						break;
					case 'd':
						passantFile = 3;
						break;
					case 'e':
						passantFile = 4;
						break;
					case 'f':
						passantFile = 5;
						break;
					case 'g':
						passantFile = 6;
						break;
					case 'h':
						passantFile = 7;
						break;
					}
					fen++;
					const int passantRank = *fen - 49;
					enPassantTarget = passantFile + passantRank*8;
				}
				else if (section == 3) {
					halfMoves = (*fen) - 48;
				}
				else if (section == 4) {
					fullMoves = (*fen) - 48;
				}
			}

			break;
		}
		fen++;
	}

	occupied = (colors[White] | colors[Black]);
	GenAttackMaps(*this);
    hashKey = GetHashKey(*this);
}

void Board::PrintBoard() {

	for (int rank = 7; rank >= 0; rank--) {

		std::cout << "+---+---+---+---+---+---+---+---+\n";
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
		std::cout << ' ' << rank + 1 << '\n';
	}

	std::cout << "+---+---+---+---+---+---+---+---+\n";
	std::cout << "  a   b   c   d   e   f   g   h\n\n";
	std::cout << "      Side to move: ";
	if (!sideToMove) {
		std::cout << "White\n";
	} else {
		std::cout << "Black\n";
	}
	if (enPassantTarget >= 0) {
		std::cout << "      En Passant square: " << squareCoords[enPassantTarget] << '\n';
	} else {
		std::cout << "      En Passant square: None" << '\n';
	}

	std::cout << "      Castling rights: ";
	if (castlingRights & whiteKingRight) std::cout << "K"; else std::cout << "-";
	if (castlingRights & whiteQueenRight) std::cout << "Q"; else std::cout << "-";
	if (castlingRights & blackKingRight) std::cout << "k"; else std::cout << "-";
	if (castlingRights & blackQueenRight) std::cout << "q"; else std::cout << "-";
	std::cout << "\n\n";

    std::cout << "      Hashkey: " << std::hex << hashKey << '\n';
}

void Board::AddMove(Move move) {
    moveList[currentMoveIndex] = move;
    currentMoveIndex++;
}

void Board::ResetMoves() {
    currentMoveIndex = 0;
}

void Board::ListMoves() {
	for (int i = 0; i < currentMoveIndex; i++) {
		std::cout << i+1 << ". ";
		moveList[i].PrintMove();
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

U64 Board::GetAttackMaps(bool side) {
	U64 combined = 0ULL;
	for (int type = Pawn; type <= King; type++) {
		combined |= attackMaps[side][type];
	}
	return combined;
}

bool Board::InCheck(bool side) {
	Bitboard myKingSquare = colors[side] & pieces[King];

	return GetAttackMaps(!side) & myKingSquare;
}

void Board::SetPiece(int piece, int square, bool color) {
	pieces[piece].SetBit(square);
	colors[color].SetBit(square);
	occupied.SetBit(square);

    hashKey ^= zKeys[color][piece][square];
}

void Board::RemovePiece(int piece, int square, bool color) {
	pieces[piece].PopBit(square);
	colors[color].PopBit(square);
	occupied.PopBit(square);

    hashKey ^= zKeys[color][piece][square];
}

// Updates castling rights
static void UpdateCastlingRights(Board &board, int square, int type, int color) {
	if (type == Rook) {
        // Removing old rights
        board.hashKey ^= zCastle[board.castlingRights];
		int queenSideRook = color ? a8 : a1;
		int kingSideRook = color ? h8 : h1;

		if (square == queenSideRook) {
            board.castlingRights &= color ? ~blackQueenRight : ~whiteQueenRight;
		} else if (square == kingSideRook) {
            board.castlingRights &= color ? ~blackKingRight : ~whiteKingRight;
		}

        board.hashKey ^= zCastle[board.castlingRights];
	} else if (type == King) {
        // Removing old rights
        board.hashKey ^= zCastle[board.castlingRights];

        board.castlingRights &= color ? ~blackKingRight : ~whiteKingRight;
        board.castlingRights &= color ? ~blackQueenRight : ~whiteQueenRight;

        board.hashKey ^= zCastle[board.castlingRights];
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
	int attackerPiece = GetPieceType(move.MoveFrom());
	int attackerColor = GetPieceColor(move.MoveFrom());

	int targetPiece = GetPieceType(move.MoveTo());
	int direction = attackerColor ? south : north;

	int newEpTarget = -1;

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

	halfMoves++;
	if (halfMoves % 2 == 0) fullMoves++;
	sideToMove = !attackerColor;
    hashKey ^= zSide;

    if (enPassantTarget != -1) {
        hashKey ^= zEnPassant[enPassantTarget % 8];
    }

    if (newEpTarget != -1) {
        hashKey ^= zEnPassant[newEpTarget % 8];
    }

	enPassantTarget = newEpTarget;

	GenAttackMaps(*this);
}
